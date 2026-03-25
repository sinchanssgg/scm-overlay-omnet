/**
 * @file SCMNode.cc
 * @brief SCM overlay node — stabilization rules, cost calculations, and cryptographic proofs
 * Author: Sinchan Sengupta <sinchan.sengupta@univ-nantes.fr>
 * Modified By: Arannya Mukherjee <arannya@adhrith.ai>
 */
#include "SCMNode.h"
#include "SCMMessages.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>
#include <fstream>
#include <sstream>
#define OPENSSL_SUPPRESS_DEPRECATED
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/sha.h>
#include <openssl/bn.h>

using namespace omnetpp;

Define_Module(SCMNode);

// ─── Constructor / Destructor ───────────────────────────────────────

SCMNode::SCMNode() : eckey(nullptr) {
    eckey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!eckey) {
        throw std::runtime_error("EC_KEY_new_by_curve_name failed — OpenSSL may not support secp256k1");
    }
    if (EC_KEY_generate_key(eckey) != 1) {
        EC_KEY_free(eckey);
        eckey = nullptr;
        throw std::runtime_error("EC_KEY_generate_key failed");
    }
}

SCMNode::~SCMNode() {
    if (eckey) EC_KEY_free(eckey);
}

// ─── OMNeT++ lifecycle ──────────────────────────────────────────────

void SCMNode::initialize()
{
    WATCH(status);
    WATCH(level);
    WATCH(beta);
    WATCH(payment);
    WATCH(subtreeSize);

    // Read NED parameters
    id = par("id");
    numUsers = par("numUsers");
    linkCost = par("linkCost");
    const char *variant = par("algorithmVariant").stringValue();
    algorithmKind = parseAlgorithmKind(variant);

    // Register signal for stabilization metrics
    stabilizationTimeSignal = registerSignal("nodeStableTime");
    lastFaultTime = 0;

    // Garg-Grosu convergence state
    prevBeta = NAN;
    ggConverged = false;
    roundCounter = 0;

    // Clear crypto state
    sizeSig.clear();
    betaSig.clear();
    proof.clear();

    // Rule 1: Root initialization
    if (id == 0) {
        parentId = -1;
        level = 0;
        status = STABLE;
        payment = 0;
        beta = 0;
        subtreeSize = numUsers;
        calculateAlpha();
        bubble("ROOT INITIALIZED");
    } else {
        // Start in faulty state (self-stabilizing from arbitrary state)
        status = FAULTY;
        level = INT_MAX;
        parentId = -1;
        beta = 0;
        payment = 0;
        subtreeSize = 0;
    }

    // Schedule first stabilization check
    scheduleAt(simTime() + uniform(0, 0.1), new cMessage("Stabilize"));
}

void SCMNode::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        handleStabilization();
        scheduleAt(simTime() + 1.0, msg);
        return;
    }

    // Track stabilization time for metrics (only after a real fault occurred)
    // Garg-Grosu uses round-count emission in handleStabilization() instead — Arannya Mukherjee
    if (algorithmKind != AlgorithmKind::GARG_GROSU &&
        status == STABLE && lastFaultTime > 0) {
        emit(stabilizationTimeSignal, (simTime() - lastFaultTime).dbl());
    }

    SCMControlMessage *ctrlMsg = dynamic_cast<SCMControlMessage*>(msg);
    if (ctrlMsg) {
        switch (ctrlMsg->getMsgType()) {
            case SCMControlMessage::ALPHA_UPDATE:
                handleAlphaUpdate(ctrlMsg);
                break;
            case SCMControlMessage::BETA_UPDATE:
                handleBetaUpdate(ctrlMsg);
                break;
            case SCMControlMessage::FAULT_NOTIFY:
                handleFaultNotification(ctrlMsg);
                break;
            case SCMControlMessage::PROOF_REQUEST:
                handleProofRequest(ctrlMsg);
                break;
            case SCMControlMessage::PROOF_RESPONSE:
                handleProofResponse(ctrlMsg);
                break;
        }
    }
    delete msg;
}

void SCMNode::refreshDisplay() const
{
    char buf[64];
    snprintf(buf, sizeof(buf), "L%d %s β=%.2f",
             level,
             status == STABLE ? "S" : (status == FAULTY ? "F" : "R"),
             beta);
    getDisplayString().setTagArg("t", 0, buf);

    // Color by status: green=STABLE, red=FAULTY, yellow=RECOVERING
    const char *color = (status == STABLE) ? "green"
                      : (status == FAULTY) ? "red"
                      : "yellow";
    getDisplayString().setTagArg("i2", 0, color);
}

void SCMNode::finish()
{
    if (id != 0) {
        return;
    }

    cModule *network = getParentModule();
    if (!network) {
        return;
    }

    const char *resultDirParam = getEnvir()->getConfig()->getConfigValue("result-dir");
    if (!resultDirParam || !*resultDirParam) {
        EV_WARN << "result-dir not set; skipping mwe_node_state.csv export" << endl;
        return;
    }

    std::string resultDir = resultDirParam;
    std::string outPath = resultDir + "/mwe_node_state.csv";
    std::ofstream out(outPath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        EV_WARN << "Cannot open " << outPath << " for writing" << endl;
        return;
    }

    std::string topologyLabel = network->getNedTypeName();
    out << "node_id,parent_id,level,num_users,subtree_size,beta,payment,proof_size_bytes,status,proof_valid,algorithm,topology,scm_local_consistent\n";
    int numNodes = network->par("numNodes").intValue();
    for (int i = 0; i < numNodes; i++) {
        cModule *mod = network->getSubmodule("node", i);
        SCMNode *node = dynamic_cast<SCMNode*>(mod);
        if (!node) {
            continue;
        }

        const char *statusLabel =
            (node->status == STABLE) ? "STABLE" :
            (node->status == FAULTY) ? "FAULTY" : "RECOVERING";

        bool scmLocalConsistent = true;
        if (node->parentId != -1) {
            SCMNode *parent = node->getParentNode();
            if (!parent || node->level != parent->level + 1) {
                scmLocalConsistent = false;
            } else if (node->subtreeSize > 0) {
                double expectedBeta = parent->beta + (node->linkCost / node->subtreeSize);
                if (fabs(node->beta - expectedBeta) > 1e-6) {
                    scmLocalConsistent = false;
                }
            }
        }

        out << node->id << ","
            << node->parentId << ","
            << node->level << ","
            << node->numUsers << ","
            << node->subtreeSize << ","
            << node->beta << ","
            << node->payment << ","
            << node->proof.size() << ","
            << statusLabel << ","
            << (node->proof.empty() ? 0 : 1) << ","
            << algorithmKindLabel(node->algorithmKind) << ","
            << topologyLabel << ","
            << (scmLocalConsistent ? 1 : 0) << "\n";
    }
    out.close();
    EV << "Wrote MWE node state CSV to " << outPath << endl;
}

// ─── Stabilization rules ────────────────────────────────────────────

void SCMNode::handleStabilization()
{
    // --- Recovery Phase ---
    // Rule 2: Error Detection (Find better parent)
    if (status == STABLE && existsBetterParent()) {
        int oldParent = parentId;
        parentId = findBestParent();
        if (parentId != oldParent) {
            SCMNode *p = getParentNode();
            if (p) level = p->level + 1;
            sizeSig.clear();
            betaSig.clear();
            calculateAlpha();
            calculateBeta();
            bubble("FOUND BETTER PARENT");
        }
    }

    // Rule 3: Error Propagation (Become FAULTY if inconsistent)
    if (status == STABLE && (notLocallyConsistent() || lostStableSupport())) {
        transitionToFaulty();
        bubble("DETECTED INCONSISTENCY");
        if (algorithmKind != AlgorithmKind::GARG_GROSU) {
            notifyChildren(SCMControlMessage::FAULT_NOTIFY);
        }
        return;
    }

    // Rule 4: Start Recovery (Become RECOVERING when children are ready)
    if (status == FAULTY &&
        (algorithmKind == AlgorithmKind::GARG_GROSU || allChildrenRecovering())) {
        status = RECOVERING;
        calculateAlpha();
        bubble("STARTING RECOVERY");
        return;
    }

    // Rule 5: Rejoin Tree (Find stable parent and become STABLE)
    if (status == RECOVERING || status == FAULTY) {
        if (rejoinTree()) {
            status = STABLE;
            calculateAlpha();
            calculateBeta();
            payment = beta * numUsers;
            // Garg-Grosu uses beta-convergence detection (below), not fault-recovery timing — Arannya Mukherjee
            if (algorithmKind != AlgorithmKind::GARG_GROSU && lastFaultTime > 0) {
                emit(stabilizationTimeSignal, (simTime() - lastFaultTime).dbl());
            }
            bubble("REJOINED TREE");
        }
        return;
    }

    // --- State Publication Phase ---
    // Rule 6: Sign and Publish State
    if (status == STABLE && (sizeSig.empty() || betaSig.empty())) {
        signState();
        bubble("STATE SIGNED");
    }

    // --- Proof Propagation Phase ---
    // Rule 7: Propagate Proof
    bool readyForProof = allChildrenHaveProofs();
    if (algorithmKind == AlgorithmKind::GARG_GROSU) {
        // Baseline path: greedy local progress without strict bottom-up dependency.
        readyForProof = true;
    }
    if (status == STABLE && readyForProof) {
        buildProof();
        if (id == 0) {
            verifyProofChain();
        }
        bubble("PROPAGATING PROOF");
    }

    // --- Garg-Grosu convergence detection --- — Arannya Mukherjee
    // Per Garg-Grosu: a node declares local convergence when its beta value
    // is identical across two consecutive rounds. Only track while STABLE
    // to avoid counting rounds spent in FAULTY/RECOVERING.
    if (algorithmKind == AlgorithmKind::GARG_GROSU && status == STABLE) {
        roundCounter++;
        if (!ggConverged && !std::isnan(prevBeta) && fabs(beta - prevBeta) < 1e-6) {
            ggConverged = true;
            emit(stabilizationTimeSignal, (double)roundCounter);
        }
        prevBeta = beta;
    }
}

// ─── State transition helpers ────────────────────────────────────────

void SCMNode::transitionToFaulty()
{
    status = FAULTY;
    lastFaultTime = simTime().dbl();
    sizeSig.clear();
    betaSig.clear();
    // Reset Garg-Grosu convergence so re-convergence is tracked after recovery — Arannya Mukherjee
    ggConverged = false;
    prevBeta = NAN;
    roundCounter = 0;
}

// ─── Consistency checks ─────────────────────────────────────────────

bool SCMNode::notLocallyConsistent()
{
    if (parentId == -1) return false;  // Root is always consistent

    SCMNode *parent = getParentNode();
    if (!parent) return true;  // Parent gone = inconsistent

    // Definition 2: level must be parent's level + 1
    if (level != parent->level + 1) return true;

    if (algorithmKind == AlgorithmKind::GARG_GROSU) {
        // Garg-Grosu baseline consistency predicate is structural.
        return false;
    }

    // Beta must match: parent_beta + linkCost / subtreeSize
    if (subtreeSize > 0) {
        double expectedBeta = parent->beta + (linkCost / subtreeSize);
        if (fabs(beta - expectedBeta) > 1e-6) return true;
    }

    return false;
}

bool SCMNode::lostStableSupport()
{
    if (parentId == -1) return false;  // Root has no parent

    SCMNode *parent = getParentNode();
    if (!parent) return true;  // Parent unreachable

    return parent->status != STABLE;
}

bool SCMNode::allChildrenRecovering()
{
    std::vector<SCMNode*> children = getChildrenNodes();
    for (SCMNode *child : children) {
        if (child->status != RECOVERING && child->status != STABLE) {
            return false;
        }
    }
    return true;  // True even if no children
}

bool SCMNode::allChildrenHaveProofs()
{
    std::vector<SCMNode*> children = getChildrenNodes();
    for (SCMNode *child : children) {
        if (child->status == STABLE && child->proof.empty()) {
            return false;
        }
    }
    return true;
}

bool SCMNode::existsBetterParent()
{
    if (parentId == -1) return false;  // Root

    int bestId = findBestParent();
    if (bestId == -1) return false;

    SCMNode *best = getNodeById(bestId);
    SCMNode *current = getParentNode();

    if (!current) return (best != nullptr);
    if (!best) return false;

    return parentScore(best) < parentScore(current);
}

int SCMNode::findBestParent()
{
    double bestScore = INFINITY;
    int bestId = -1;

    // Iterate over all connected neighbours via gates
    for (int i = 0; i < gateSize("port$o"); i++) {
        cGate *g = gate("port$o", i);
        if (!g->isConnected()) continue;

        cModule *neighbor = g->getNextGate()->getOwnerModule();
        SCMNode *nNode = dynamic_cast<SCMNode*>(neighbor);
        if (!nNode) continue;

        if (nNode->status == STABLE && nNode->id != id) {
            double score = parentScore(nNode);
            if (score < bestScore) {
                bestScore = score;
                bestId = nNode->id;
            }
        }
    }
    return bestId;
}

double SCMNode::parentScore(const SCMNode* candidate) const
{
    if (!candidate) {
        return INFINITY;
    }
    if (algorithmKind == AlgorithmKind::GARG_GROSU) {
        return candidate->beta;
    }
    if (algorithmKind == AlgorithmKind::BYRENHEID) {
        return static_cast<double>(candidate->level) * 1000000.0 + candidate->id;
    }
    return candidate->level;
}

// ─── Tree operations ────────────────────────────────────────────────

bool SCMNode::rejoinTree()
{
    int newParentId = findBestParent();
    if (newParentId == -1) return false;

    SCMNode *newParent = getNodeById(newParentId);
    if (newParent && newParent->status == STABLE) {
        parentId = newParentId;
        level = newParent->level + 1;
        return true;
    }
    return false;
}

void SCMNode::notifyChildren(SCMControlMessage::MsgType msgType)
{
    for (int i = 0; i < gateSize("port$o"); i++) {
        cGate *g = gate("port$o", i);
        if (!g->isConnected()) continue;

        cModule *neighbor = g->getNextGate()->getOwnerModule();
        SCMNode *nNode = dynamic_cast<SCMNode*>(neighbor);
        if (nNode && nNode->parentId == id) {
            SCMControlMessage *msg = new SCMControlMessage("Notify");
            msg->setMsgType(msgType);
            msg->setSenderId(id);
            send(msg, "port$o", i);
        }
    }
}

// ─── Cost calculations ──────────────────────────────────────────────

void SCMNode::calculateAlpha()
{
    // Alpha = total users in subtree (this node + children subtrees)
    subtreeSize = numUsers;
    for (SCMNode *child : getChildrenNodes()) {
        subtreeSize += child->subtreeSize;
    }
}

void SCMNode::calculateBeta()
{
    // Beta = parent's beta + linkCost / subtreeSize
    if (parentId == -1) {
        beta = 0;  // Root pays nothing upstream
        return;
    }

    SCMNode *parent = getParentNode();
    if (parent && subtreeSize > 0) {
        beta = parent->beta + (linkCost / subtreeSize);
    }
    payment = beta * numUsers;
}

double SCMNode::calculateMaxGain()
{
    double maxGain = linkCost;
    for (SCMNode *child : getChildrenNodes()) {
        maxGain += child->calculateMaxGain();
    }
    return maxGain;
}

// ─── Message handlers ───────────────────────────────────────────────

void SCMNode::handleAlphaUpdate(SCMControlMessage* msg)
{
    // Parent informs us of updated subtree size
    int senderSubtreeSize = (int)msg->getValue();
    calculateAlpha();
    calculateBeta();
}

void SCMNode::handleBetaUpdate(SCMControlMessage* msg)
{
    // Parent informs us of new beta; recalculate ours
    calculateBeta();
    payment = beta * numUsers;
}

void SCMNode::handleFaultNotification(SCMControlMessage* msg)
{
    if (status == STABLE) {
        transitionToFaulty();
        bubble("FAULT RECEIVED");
        notifyChildren(SCMControlMessage::FAULT_NOTIFY);
    }
}

void SCMNode::handleProofRequest(SCMControlMessage* msg)
{
    // Respond with our current proof
    SCMControlMessage *resp = new SCMControlMessage("ProofResponse");
    resp->setMsgType(SCMControlMessage::PROOF_RESPONSE);
    resp->setSenderId(id);
    // In a full implementation, attach serialized proof data.
    // For now, respond with proof availability indicator.
    resp->setValue(proof.empty() ? 0.0 : 1.0);

    // Send back to requester via direct message
    cModule *requester = getNodeById(msg->getSenderId());
    if (requester) {
        sendDirect(resp, requester->gate("port$i", 0));
    } else {
        delete resp;
    }
}

void SCMNode::handleProofResponse(SCMControlMessage* msg)
{
    // Received proof from child — used during proof propagation phase
    // The actual proof verification is done in verifyProofChain()
    EV << "Node " << id << " received proof response from node " << msg->getSenderId() << endl;
}

// ─── Cryptographic operations ───────────────────────────────────────

void SCMNode::signState()
{
    std::string sizeStr = std::to_string(subtreeSize);
    sizeSig = signMessage(sizeStr);

    std::string betaStr = std::to_string(beta);
    betaSig = signMessage(betaStr);
}

std::vector<uint8_t> SCMNode::signMessage(const std::string& message)
{
    // SHA-256 hash of message
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(message.c_str()),
           message.size(), hash);

    // ECDSA sign
    int sigLenSigned = ECDSA_size(eckey);
    if (sigLenSigned <= 0) {
        EV_WARN << "Node " << id << ": ECDSA_size failed" << endl;
        return {};
    }
    unsigned int sigLen = (unsigned int)sigLenSigned;
    std::vector<uint8_t> sig(sigLen);
    if (ECDSA_sign(0, hash, SHA256_DIGEST_LENGTH, sig.data(), &sigLen, eckey) != 1) {
        EV_WARN << "Node " << id << ": ECDSA_sign failed" << endl;
        return {};
    }
    sig.resize(sigLen);
    return sig;
}

bool SCMNode::verifySignature(const std::string& message,
                              const std::vector<uint8_t>& signature,
                              EC_KEY* key)
{
    if (signature.empty() || !key) return false;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(message.c_str()),
           message.size(), hash);

    return ECDSA_verify(0, hash, SHA256_DIGEST_LENGTH,
                        signature.data(), (int)signature.size(), key) == 1;
}

std::string SCMNode::serializeProof(const ProofStruct& proofData)
{
    std::ostringstream oss;
    oss << proofData.nodeId << ":"
        << proofData.numUsers << ":"
        << proofData.subtreeSize << ":"
        << proofData.betaValue << ":"
        << proofData.payment;
    return oss.str();
}

ProofStruct SCMNode::deserializeProof(const std::string& proofStr)
{
    ProofStruct p;
    std::istringstream iss(proofStr);
    char sep;
    iss >> p.nodeId >> sep
        >> p.numUsers >> sep
        >> p.subtreeSize >> sep
        >> p.betaValue >> sep
        >> p.payment;
    return p;
}

// ─── Proof propagation ──────────────────────────────────────────────

void SCMNode::buildProof()
{
    ProofStruct proofData;
    proofData.nodeId = id;
    proofData.numUsers = numUsers;
    proofData.subtreeSize = subtreeSize;
    proofData.betaValue = beta;
    proofData.payment = payment;

    // Add parent's beta signature
    if (parentId != -1) {
        SCMNode *parent = getParentNode();
        if (parent) proofData.parentBetaSig = parent->betaSig;
    }

    // Collect children's size signatures and proofs
    for (SCMNode *child : getChildrenNodes()) {
        if (child->status == STABLE) {
            proofData.childrenSigs.push_back(child->sizeSig);
            if (!child->proof.empty()) {
                proofData.childProofs.push_back(child->proof);
            }
        }
    }

    // Sign the complete proof structure
    std::string proofStr = serializeProof(proofData);
    proof = signMessage(proofStr);
}

void SCMNode::verifyProofChain()
{
    if (id != 0) return;

    bool verificationPassed = true;

    for (SCMNode *child : getChildrenNodes()) {
        if (!verifyNodeProof(child)) {
            verificationPassed = false;
            handleCheatingNode(child->id);
        }
    }

    if (verificationPassed) {
        bubble("PROOF VERIFICATION SUCCESSFUL");
    } else {
        bubble("PROOF VERIFICATION FAILED - CHEATING DETECTED");
    }
}

bool SCMNode::verifyNodeProof(SCMNode *node)
{
    // 1. Verify node's own signatures
    std::string sizeStr = std::to_string(node->subtreeSize);
    if (!verifySignature(sizeStr, node->sizeSig, node->eckey)) {
        return false;
    }

    std::string betaStr = std::to_string(node->beta);
    if (!verifySignature(betaStr, node->betaSig, node->eckey)) {
        return false;
    }

    // 2. Verify parent-child beta consistency
    if (node->parentId != -1) {
        SCMNode *parent = node->getParentNode();
        if (parent && node->subtreeSize > 0) {
            double expectedBeta = parent->beta + (node->linkCost / node->subtreeSize);
            if (fabs(node->beta - expectedBeta) > 1e-6) {
                return false;
            }
        }
    }

    // 3. Verify subtree size consistency
    int calculatedSize = node->numUsers;
    for (SCMNode *child : node->getChildrenNodes()) {
        calculatedSize += child->subtreeSize;
    }
    if (calculatedSize != node->subtreeSize) {
        return false;
    }

    return true;
}

void SCMNode::handleCheatingNode(int cheatingNodeId)
{
    SCMNode *cheater = getNodeById(cheatingNodeId);
    if (!cheater) return;

    // Impose financial penalty
    cheater->payment += 2 * cheater->calculateMaxGain();

    // Mark as faulty and invalidate crypto state
    cheater->status = FAULTY;
    cheater->sizeSig.clear();
    cheater->betaSig.clear();
    cheater->proof.clear();

    bubble(("PUNISHING CHEATING NODE " + std::to_string(cheatingNodeId)).c_str());
}

// ─── Utility: node lookups ──────────────────────────────────────────

SCMNode* SCMNode::getParentNode()
{
    if (parentId == -1) return nullptr;
    return getNodeById(parentId);
}

std::vector<SCMNode*> SCMNode::getChildrenNodes()
{
    std::vector<SCMNode*> children;
    // Children are connected neighbours whose parentId == our id
    for (int i = 0; i < gateSize("port$o"); i++) {
        cGate *g = gate("port$o", i);
        if (!g->isConnected()) continue;

        cModule *neighbor = g->getNextGate()->getOwnerModule();
        SCMNode *nNode = dynamic_cast<SCMNode*>(neighbor);
        if (nNode && nNode->parentId == id) {
            children.push_back(nNode);
        }
    }
    return children;
}

SCMNode* SCMNode::getNodeById(int nodeId)
{
    cModule *network = getParentModule();
    if (!network) return nullptr;
    cModule *mod = network->getSubmodule("node", nodeId);
    return dynamic_cast<SCMNode*>(mod);
}

SCMNode::AlgorithmKind SCMNode::parseAlgorithmKind(const char* variant)
{
    if (!variant || strcmp(variant, "scm") == 0) {
        return AlgorithmKind::SCM;
    }
    if (strcmp(variant, "garg-grosu") == 0) {
        return AlgorithmKind::GARG_GROSU;
    }
    if (strcmp(variant, "byrenheid") == 0) {
        return AlgorithmKind::BYRENHEID;
    }
    throw cRuntimeError("Unsupported algorithmVariant '%s'", variant);
}

const char* SCMNode::algorithmKindLabel(AlgorithmKind kind)
{
    switch (kind) {
        case AlgorithmKind::SCM:
            return "SCM";
        case AlgorithmKind::GARG_GROSU:
            return "Garg-Grosu";
        case AlgorithmKind::BYRENHEID:
            return "Byrenheid";
    }
    return "Unknown";
}
