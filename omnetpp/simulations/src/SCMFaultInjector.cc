#include "SCMFaultInjector.h"
#include "SCMNode.h"
#include "SCMMessages.h"
#include <algorithm>
#include <cmath>

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

void SCMFaultInjector::buildDepthBuckets(int numNodes)
{
    depthBuckets.clear();
    cModule *network = getParentModule();

    // First pass: find actual max depth from node levels
    int maxDepth = 0;
    for (int i = 0; i < numNodes; i++) {
        SCMNode *node = check_and_cast<SCMNode*>(network->getSubmodule("node", i));
        int depth = node->getLevel();
        if (depth >= 0 && depth < 1000000) {
            maxDepth = std::max(maxDepth, depth);
        }
    }

    // Fall back to CBT estimate if no valid levels found
    if (maxDepth == 0) {
        maxDepth = computeCbtDepthFromIndex(numNodes - 1);
    }

    depthBuckets.resize(maxDepth + 1);

    // Second pass: bucket nodes by their actual level
    for (int i = 0; i < numNodes; i++) {
        SCMNode *node = check_and_cast<SCMNode*>(network->getSubmodule("node", i));
        int depth = node->getLevel();
        if (depth < 0 || depth >= (int)depthBuckets.size()) {
            // Skip nodes with invalid/sentinel levels (e.g., FAULTY nodes at INT_MAX)
            continue;
        }
        depthBuckets[depth].push_back(i);
    }
}

std::vector<int> SCMFaultInjector::selectDeterministicTargets() const
{
    std::vector<int> targets;
    for (int depth = 0; depth < (int)depthBuckets.size(); depth++) {
        if (depth == 0) {
            continue;  // Never corrupt root
        }
        // campaignTargetDepth >= 0: corrupt ONLY at that exact depth
        if (campaignTargetDepth >= 0 && depth != campaignTargetDepth) {
            continue;
        }
        // campaignTargetDepth < 0: fall back to maxCampaignDepth range behavior
        if (campaignTargetDepth < 0 && maxCampaignDepth >= 0 && depth > maxCampaignDepth) {
            continue;
        }
        const auto &bucket = depthBuckets[depth];
        if (bucket.empty()) {
            continue;
        }
        int idx = (campaignSeed + campaignRound + depth) % (int)bucket.size();
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

void SCMFaultInjector::applyFaultToNode(SCMNode *node, int numNodes)
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
}

void SCMFaultInjector::initialize()
{
    faultType = (FaultType)par("faultType").intValue();
    campaignMode = (CampaignMode)par("campaignMode").intValue();
    campaignSeed = par("campaignSeed").intValue();
    campaignRound = 0;
    maxCampaignDepth = par("maxCampaignDepth").intValue();
    campaignTargetDepth = par("campaignTargetDepth").intValue();
    parentOffset = par("parentOffset").intValue();
    strictDepthCampaign = par("strictDepthCampaign").boolValue();
    sendFaultNotify = par("sendFaultNotify").boolValue();

    if (parentOffset < 1) {
        throw cRuntimeError("parentOffset must be >= 1");
    }

    scheduleAt(simTime() + par("initialDelay").doubleValue(), 
              new cMessage("InjectFault"));
}

void SCMFaultInjector::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage() && strcmp(msg->getName(), "InjectFault") == 0) {
        injectFault();
        campaignRound++;
        scheduleAt(simTime() + par("interval").doubleValue(), msg);
    } else {
        delete msg;
    }
}

void SCMFaultInjector::injectFault()
{
    cModule *network = getParentModule();
    int numNodes = network->par("numNodes");

    if (campaignMode == DETERMINISTIC_ONE_NODE_PER_DEPTH) {
        const char *networkName = network->getNedTypeName();
        bool cbtLike = std::string(networkName) == "CompleteBinaryTree";
        // When campaignTargetDepth is set, allow any network type
        if (!cbtLike && campaignTargetDepth < 0) {
            if (strictDepthCampaign) {
                throw cRuntimeError("Deterministic depth campaign requires CompleteBinaryTree network; got %s", networkName);
            }
            EV_WARN << "Skipping deterministic depth campaign on non-CBT network " << networkName << endl;
            return;
        }
        buildDepthBuckets(numNodes);
        auto targets = selectDeterministicTargets();
        for (int nodeId : targets) {
            SCMNode *node = check_and_cast<SCMNode*>(network->getSubmodule("node", nodeId));
            applyFaultToNode(node, numNodes);
        }
        return;
    }

    for (int i = 0; i < numNodes; i++) {
        if (uniform(0, 1) < par("faultProbability").doubleValue()) {
            SCMNode *node = check_and_cast<SCMNode*>(network->getSubmodule("node", i));
            applyFaultToNode(node, numNodes);
        }
    }
}
