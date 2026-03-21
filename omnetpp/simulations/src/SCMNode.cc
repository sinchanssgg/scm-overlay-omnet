#include "SCMNode.h"
#include "SCMMessages.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include "openssl/ec.h"
#include "openssl/ecdsa.h"
#include "openssl/obj_mac.h"
#include "openssl/sha.h"

Define_Module(SCMNode);

// Constructor: Initialize crypto context
SCMNode::SCMNode() {
    eckey = EC_KEY_new_by_curve_name(NID_secp256k1);
    EC_KEY_generate_key(eckey);
}

// Destructor: Clean up crypto context
SCMNode::~SCMNode() {
    EC_KEY_free(eckey);
}

void SCMNode::initialize()
{
    WATCH(status);
    WATCH(level);
    WATCH(beta);
    WATCH(payment);
    WATCH(subtreeSize);
    
    // Initialize node parameters
    id = par("id");
    numUsers = par("numUsers");
    linkCost = par("linkCost");
    
    // Initialize crypto and proof variables
    sizeSig = nullptr;
    betaSig = nullptr;
    proof = nullptr;
    
    // Rule 1: Root initialization
    if (id == 0) { // Node 0 is the root
        parentId = -1;
        level = 0;
        status = STABLE;
        payment = 0;
        beta = 0;
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
        scheduleAt(simTime() + 1.0, msg); // Check every 1 second
        return;
    }

    // Track stable time for metrics
    if (status == STABLE) {
        simsignal_t sig = registerSignal("nodeStableTime");
        emit(sig, simTime() - lastFaultTime);
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

void SCMNode::handleStabilization()
{
    // --- Recovery Phase ---
    // Rule 2: Error Detection (Find better parent)
    if (status == STABLE && existsBetterParent()) {
        int oldParent = parentId;
        parentId = findBestParent();
        if (parentId != oldParent) {
            level = getParentNode()->level + 1;
            sizeSig = nullptr;  // Invalidate signatures on parent change
            betaSig = nullptr;
            calculateAlpha();
            calculateBeta();
            bubble("FOUND BETTER PARENT");
        }
    }
    
    // Rule 3: Error Propagation (Become FAULTY if inconsistent)
    if (status == STABLE && (notLocallyConsistent() || lostStableSupport())) {
        status = FAULTY;
        sizeSig = nullptr;
        betaSig = nullptr;
        bubble("DETECTED INCONSISTENCY");
        notifyChildren(SCMControlMessage::FAULT_NOTIFY);
        return;
    }
    
    // Rule 4: Start Recovery (Become RECOVERING when children are ready)
    if (status == FAULTY && allChildrenRecovering()) {
        status = RECOVERING;
        calculateAlpha();  // Recalculate based on children's state
        bubble("STARTING RECOVERY");
        return;
    }
    
    // Rule 5: Rejoin Tree (Find stable parent and become STABLE)
    if (status == RECOVERING || status == FAULTY) {
        if (rejoinTree()) {
            // Successfully rejoined - now proceed to state publication
            status = STABLE;
            calculateAlpha();
            calculateBeta();
            payment = beta * numUsers;
            bubble("REJOINED TREE");
        }
        return;
    }
    
    // --- State Publication Phase ---
    // Rule 6: Sign and Publish State (Create cryptographic signatures)
    if (status == STABLE && (sizeSig == nullptr || betaSig == nullptr)) {
        signState();
        bubble("STATE SIGNED");
    }
    
    // --- Proof Propagation Phase ---
    // Rule 7: Propagate Proof (Build proof when children have proofs)
    if (status == STABLE && allChildrenHaveProofs()) {
        buildProof();
        if (id == 0) { // Root initiates audit
            verifyProofChain();
        }
        bubble("PROPAGATING PROOF");
    }
}

bool SCMNode::rejoinTree()
{
    int newParentId = findBestParent();
    if (newParentId != -1) {
        SCMNode *parent = getParentNode();
        if (parent && parent->status == STABLE) {
            parentId = newParentId;
            level = parent->level + 1;
            return true;
        }
    }
    return false;
}

void SCMNode::signState()
{
    // Sign subtree size
    std::string sizeStr = std::to_string(subtreeSize);
    sizeSig = signMessage(sizeStr);
    
    // Sign beta value
    std::string betaStr = std::to_string(beta);
    betaSig = signMessage(betaStr);
}

void SCMNode::buildProof()
{
    // Create proof structure according to Algorithm 2, Rule 7
    ProofStruct proofData;
    proofData.nodeId = id;
    proofData.numUsers = numUsers;
    proofData.subtreeSize = subtreeSize;
    proofData.betaValue = beta;
    proofData.payment = payment;
    
    // Add parent's beta signature
    if (parentId != -1) {
        SCMNode *parent = getParentNode();
        proofData.parentBetaSig = parent->betaSig;
    }
    
    // Collect children's size signatures
    for (SCMNode *child : getChildrenNodes()) {
        if (child->status == STABLE) {
            proofData.childrenSigs.push_back(child->sizeSig);
        }
    }
    
    // Sign the complete proof structure
    std::string proofStr = serializeProof(proofData);
    proof = signMessage(proofStr);
    
    // Append children's proofs (building the chain)
    for (SCMNode *child : getChildrenNodes()) {
        if (child->proof != nullptr) {
            proofData.childProofs.push_back(child->proof);
        }
    }
}

void SCMNode::verifyProofChain()
{
    if (id != 0) return; // Only root verifies
    
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
    // Verify the cryptographic proof for a specific node
    // 1. Verify node's own signatures
    std::string sizeStr = std::to_string(node->subtreeSize);
    if (!verifySignature(sizeStr, node->sizeSig, node->eckey)) {
        return false;
    }
    
    std::string betaStr = std::to_string(node->beta);
    if (!verifySignature(betaStr, node->betaSig, node->eckey)) {
        return false;
    }
    
    // 2. Verify parent-child relationship consistency
    if (node->parentId != -1) {
        SCMNode *parent = node->getParentNode();
        double expectedBeta = parent->beta + (node->linkCost / node->subtreeSize);
        if (fabs(node->beta - expectedBeta) > 1e-6) {
            return false;
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
    // Implement punishment mechanism
    // 1. Impose financial penalty
    SCMNode *cheater = getNodeById(cheatingNodeId);
    cheater->payment += 2 * cheater->calculateMaxGain();
    
    // 2. Optionally isolate node or trigger recovery
    cheater->status = FAULTY;
    cheater->sizeSig = nullptr;
    cheater->betaSig = nullptr;
    cheater->proof = nullptr;
    
    bubble(("PUNISHING CHEATING NODE " + std::to_string(cheatingNodeId)).c_str());
}

double SCMNode::calculateMaxGain()
{
    // Calculate G_max^u = sum of costs in subtree
    double maxGain = linkCost;
    for (SCMNode *child : getChildrenNodes()) {
        maxGain += child->calculateMaxGain();
    }
    return maxGain;
}
