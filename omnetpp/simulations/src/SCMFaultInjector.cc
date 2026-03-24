#include "SCMFaultInjector.h"
#include "SCMNode.h"
#include "SCMMessages.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>

using namespace omnetpp;

Define_Module(SCMFaultInjector);

int SCMFaultInjector::computeCbtDepthFromIndex(int nodeIndex) const
{
    int depth = 0;
    int index = nodeIndex + 1;
    while (index > 1) {
        index >>= 1;
        depth++;
    }
    return depth;
}

void SCMFaultInjector::buildDepthBuckets(int numNodes, bool allowCbtFallback)
{
    depthBuckets.clear();
    int maxDepth = 0;
    std::vector<int> resolved(numNodes, -1);
    cModule *network = getParentModule();
    for (int i = 0; i < numNodes; i++) {
        SCMNode *node = check_and_cast<SCMNode*>(network->getSubmodule("node", i));
        int depth = node->getLevel();
        if (depth < 0 || depth >= numNodes) {
            if (!allowCbtFallback) {
                if (strictDepthCampaign) {
                    throw cRuntimeError("Node %d has unresolved SCM level for deterministic campaign", i);
                }
                continue;
            }
            depth = computeCbtDepthFromIndex(i);
        }
        resolved[i] = depth;
        maxDepth = std::max(maxDepth, depth);
    }
    depthBuckets.resize(std::max(1, maxDepth + 1));
    for (int i = 0; i < numNodes; i++) {
        if (resolved[i] < 0) {
            continue;
        }
        depthBuckets[resolved[i]].push_back(i);
    }
}

std::vector<int> SCMFaultInjector::selectDeterministicTargets() const
{
    std::vector<int> targets;
    for (int depth = 0; depth < (int)depthBuckets.size(); depth++) {
        if (depth == 0) {
            continue;  // Never corrupt root
        }
        if (maxCampaignDepth >= 0 && depth > maxCampaignDepth) {
            continue;
        }
        if (campaignExactLevel >= 0 && depth != campaignExactLevel) {
            continue;
        }
        const auto &bucket = depthBuckets[depth];
        if (bucket.empty()) {
            continue;
        }
        int idx = (campaignSeed + depth) % (int)bucket.size();
        targets.push_back(bucket[idx]);
    }
    return targets;
}

void SCMFaultInjector::notifyNodeFault(SCMNode *node)
{
    if (!sendFaultNotify) {
        return;
    }
    SCMControlMessage *faultMsg = new SCMControlMessage("FaultNotify");
    faultMsg->setMsgType(SCMControlMessage::FAULT_NOTIFY);
    faultMsg->setSenderId(-1);  // From fault injector
    sendDirect(faultMsg, node->gate("faultIn"));
}

void SCMFaultInjector::scheduleMetricSample(int nodeId, int level, double baselineBeta, double baselinePayment)
{
    if (!enableMetricSampling) {
        return;
    }
    cMessage *sample = new cMessage("SampleMetrics");
    sample->addPar("nodeId") = nodeId;
    sample->addPar("corruptionLevel") = level;
    sample->addPar("baselineBeta") = baselineBeta;
    sample->addPar("baselinePayment") = baselinePayment;
    sample->addPar("serial") = sampleSerial++;
    scheduleAt(simTime() + sampleDelay, sample);
}

void SCMFaultInjector::handleMetricSample(cMessage *msg)
{
    cModule *network = getParentModule();
    if (!network) {
        delete msg;
        return;
    }
    int nodeId = (int)msg->par("nodeId").longValue();
    int corruptionLevel = (int)msg->par("corruptionLevel").longValue();
    double baselineBeta = msg->par("baselineBeta").doubleValue();
    double baselinePayment = msg->par("baselinePayment").doubleValue();
    int serial = (int)msg->par("serial").longValue();

    SCMNode *node = check_and_cast<SCMNode*>(network->getSubmodule("node", nodeId));
    double sampledBeta = node->getBeta();
    double sampledPayment = node->getPayment();
    double betaPct = 0.0;
    if (fabs(baselineBeta) > 1e-12) {
        betaPct = ((sampledBeta - baselineBeta) / fabs(baselineBeta)) * 100.0;
    }
    double paymentPct = 0.0;
    if (fabs(baselinePayment) > 1e-12) {
        paymentPct = ((sampledPayment - baselinePayment) / fabs(baselinePayment)) * 100.0;
    }

    int numNodes = network->par("numNodes").intValue();
    int served = 0;
    for (int i = 0; i < numNodes; i++) {
        SCMNode *n = check_and_cast<SCMNode*>(network->getSubmodule("node", i));
        if (std::string(n->getStatusLabel()) == "STABLE" && !n->getProof().empty()) {
            served++;
        }
    }
    double serviceFraction = numNodes > 0 ? (double)served / (double)numNodes : 0.0;

    std::ostringstream row;
    row << serial << ","
        << std::fixed << std::setprecision(6) << simTime().dbl() << ","
        << nodeId << ","
        << corruptionLevel << ","
        << baselineBeta << ","
        << sampledBeta << ","
        << betaPct << ","
        << baselinePayment << ","
        << sampledPayment << ","
        << paymentPct << ","
        << serviceFraction;
    sampleRows.push_back(row.str());
    delete msg;
}

void SCMFaultInjector::applyFaultToNode(SCMNode *node, int numNodes, int corruptionLevel, double baselineBetaAvg, double baselinePaymentAvg)
{
    switch (faultType) {
        case DISTANCE_TAMPER:
            node->setLevel(node->getLevel() + 1);
            node->bubble("DISTANCE TAMPERED");
            break;

        case BETA_MODIFICATION:
            node->setBeta(node->getBeta() * 1.5);
            node->bubble("BETA MODIFIED");
            break;

        case PARENT_SWITCH:
            if (node->getId() != 0) { // Don't tamper with root
                int newParent = (node->getParentId() + parentOffset) % numNodes;
                if (newParent == node->getId()) {
                    newParent = (newParent + 1) % numNodes;
                }
                node->setParentId(newParent);
                node->bubble("PARENT SWITCHED");
            }
            break;
    }
    notifyNodeFault(node);
    scheduleMetricSample(node->getId(), corruptionLevel, baselineBetaAvg, baselinePaymentAvg);
}

void SCMFaultInjector::initialize()
{
    faultType = (FaultType)par("faultType").intValue();
    campaignMode = (CampaignMode)par("campaignMode").intValue();
    campaignSeed = par("campaignSeed").intValue();
    campaignRound = 0;
    maxCampaignDepth = par("maxCampaignDepth").intValue();
    campaignExactLevel = par("campaignExactLevel").intValue();
    parentOffset = par("parentOffset").intValue();
    strictDepthCampaign = par("strictDepthCampaign").boolValue();
    sendFaultNotify = par("sendFaultNotify").boolValue();
    oneShotCampaign = par("oneShotCampaign").boolValue();
    enableMetricSampling = par("enableMetricSampling").boolValue();
    sampleDelay = par("sampleDelay").doubleValue();
    sampleSerial = 0;

    const char *resultDirParam = getEnvir()->getConfig()->getConfigValue("result-dir");
    resultDir = (resultDirParam && *resultDirParam) ? resultDirParam : "";

    if (parentOffset < 1) {
        throw cRuntimeError("parentOffset must be >= 1");
    }
    if (campaignExactLevel < -1) {
        throw cRuntimeError("campaignExactLevel must be >= -1");
    }
    if (sampleDelay < 0.0) {
        throw cRuntimeError("sampleDelay must be >= 0");
    }

    scheduleAt(simTime() + par("initialDelay").doubleValue(), 
              new cMessage("InjectFault"));
}

void SCMFaultInjector::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage() && strcmp(msg->getName(), "InjectFault") == 0) {
        injectFault();
        campaignRound++;
        if (oneShotCampaign) {
            delete msg;
        } else {
            scheduleAt(simTime() + par("interval").doubleValue(), msg);
        }
    } else if (msg->isSelfMessage() && strcmp(msg->getName(), "SampleMetrics") == 0) {
        handleMetricSample(msg);
    } else {
        delete msg;
    }
}

void SCMFaultInjector::injectFault()
{
    cModule *network = getParentModule();
    int numNodes = network->par("numNodes");
    double baselineBetaAvg = 0.0;
    double baselinePaymentAvg = 0.0;
    int baselineCount = 0;
    for (int i = 0; i < numNodes; i++) {
        SCMNode *node = check_and_cast<SCMNode*>(network->getSubmodule("node", i));
        baselineBetaAvg += node->getBeta();
        baselinePaymentAvg += node->getPayment();
        baselineCount++;
    }
    if (baselineCount > 0) {
        baselineBetaAvg /= baselineCount;
        baselinePaymentAvg /= baselineCount;
    }

    if (campaignMode == DETERMINISTIC_ONE_NODE_PER_DEPTH) {
        const char *networkName = network->getNedTypeName();
        bool cbtLike = std::string(networkName) == "CompleteBinaryTree";
        buildDepthBuckets(numNodes, cbtLike);
        auto targets = selectDeterministicTargets();
        if (targets.empty()) {
            if (strictDepthCampaign) {
                throw cRuntimeError("Deterministic depth campaign selected no targets for network %s (exactLevel=%d, maxDepth=%d)",
                                    networkName, campaignExactLevel, maxCampaignDepth);
            }
            // Best-effort fallback for sparse/non-tree overlays: choose one deterministic
            // non-root node from the requested index-derived depth bucket so one-shot
            // campaigns always emit a sample row.
            std::vector<int> fallbackCandidates;
            if (campaignExactLevel >= 0) {
                for (int i = 1; i < numNodes; i++) {
                    if (computeCbtDepthFromIndex(i) == campaignExactLevel) {
                        fallbackCandidates.push_back(i);
                    }
                }
            }
            if (fallbackCandidates.empty()) {
                for (int i = 1; i < numNodes; i++) {
                    fallbackCandidates.push_back(i);
                }
            }
            if (!fallbackCandidates.empty()) {
                int idx = (campaignSeed + campaignRound) % (int)fallbackCandidates.size();
                targets.push_back(fallbackCandidates[idx]);
                EV_WARN << "No SCM-depth target for network " << networkName
                        << " at requested level " << campaignExactLevel
                        << "; using index-depth fallback node " << targets.front() << endl;
            }
        }
        for (int nodeId : targets) {
            SCMNode *node = check_and_cast<SCMNode*>(network->getSubmodule("node", nodeId));
            int level = campaignExactLevel >= 0 ? campaignExactLevel : node->getLevel();
            if (level < 0 || level >= numNodes) {
                level = computeCbtDepthFromIndex(nodeId);
            }
            applyFaultToNode(node, numNodes, level, baselineBetaAvg, baselinePaymentAvg);
        }
        return;
    }

    for (int i = 0; i < numNodes; i++) {
        if (uniform(0, 1) < par("faultProbability").doubleValue()) {
            SCMNode *node = check_and_cast<SCMNode*>(network->getSubmodule("node", i));
            int level = node->getLevel();
            if (level < 0 || level >= numNodes) {
                level = computeCbtDepthFromIndex(i);
            }
            applyFaultToNode(node, numNodes, level, baselineBetaAvg, baselinePaymentAvg);
        }
    }
}

void SCMFaultInjector::finish()
{
    if (!enableMetricSampling || resultDir.empty()) {
        return;
    }
    std::ofstream out(resultDir + "/fault_samples.csv", std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        EV_WARN << "Cannot open fault_samples.csv for writing in " << resultDir << endl;
        return;
    }
    out << "sample_id,sim_time,node_id,corruption_level,baseline_beta,sampled_beta,beta_pct_increase,baseline_payment,sampled_payment,payment_pct_increase,service_fraction\n";
    for (const auto &row : sampleRows) {
        out << row << "\n";
    }
}
