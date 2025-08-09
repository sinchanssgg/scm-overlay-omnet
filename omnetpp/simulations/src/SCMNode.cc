#include "SCMNode.h"
#include "SCMMessages.h"
#include <algorithm>
#include <climits>

Define_Module(SCMNode);

void SCMNode::initialize()
{
    WATCH(status);
    WATCH(level);
    WATCH(beta);
    WATCH(payment);
    
    // Initialize node parameters
    id = par("id");
    numUsers = par("numUsers");
    linkCost = par("linkCost");
    
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
        // Start in faulty state
        status = FAULTY;
        level = INT_MAX;
        parentId = -1;
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
        }
    }
    delete msg;
}

void SCMNode::handleStabilization()
{
    // Rule 2: Error detection
    if (id != 0 && notLocallyConsistent()) {
        status = FAULTY;
        bubble("DETECTED INCONSISTENCY");
        notifyChildren();
        return;
    }
    
    // Rule 3: Error propagation
    if (status == STABLE && lostStableSupport()) {
        status = FAULTY;
        bubble("LOST STABLE SUPPORT");
        notifyChildren();
        return;
    }
    
    // Rule 4: Start recovery phase
    if (status == FAULTY && allChildrenRecovering()) {
        status = RECOVERING;
        bubble("STARTING RECOVERY");
        return;
    }
    
    // Rule 5: Rejoin after recovery
    if ((status == RECOVERING || 
        (status == STABLE && existsBetterParent())) {
        rejoinTree();
    }
}

bool SCMNode::notLocallyConsistent()
{
    if (parentId == -1) return false; // Root is always consistent
    
    cModule *parent = getParentModule();
    if (!parent) return true;
    
    SCMNode *parentNode = check_and_cast<SCMNode*>(parent);
    
    // Check all conditions from Definition 2
    if (level != parentNode->level + 1) return true;
    if (status != parentNode->status && parentNode->status != FAULTY) return true;
    
    double expectedBeta = parentNode->beta + (linkCost / parentNode->subtreeSize);
    if (fabs(beta - expectedBeta) > 1e-6) return true;
    
    return false;
}

void SCMNode::rejoinTree()
{
    // Find best parent (STABLE with minimal level)
    int bestLevel = INT_MAX;
    int bestParentId = -1;
    
    for (cModule::GateIterator it(this); !it.end(); it++) {
        cGate *gate = *it;
        if (gate->getType() == cGate::INPUT && gate->isConnected()) {
            cModule *neighbor = gate->getPreviousGate()->getOwnerModule();
            SCMNode *neighborNode = check_and_cast<SCMNode*>(neighbor);
            
            if (neighborNode->status == STABLE && 
                neighborNode->level < bestLevel) {
                bestLevel = neighborNode->level;
                bestParentId = neighborNode->id;
            }
        }
    }
    
    if (bestParentId != -1) {
        parentId = bestParentId;
        level = bestLevel + 1;
        status = STABLE;
        calculateAlpha();
        calculateBeta();
        payment = beta * numUsers;
        bubble("REJOINED TREE");
    }
}