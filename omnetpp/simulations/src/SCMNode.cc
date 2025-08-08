#include "SCMNode.h"
#include <algorithm>

Define_Module(SCMNode);

void SCMNode::initialize() {
    nodeId = getIndex();  // OMNeT++ provides unique index
    numUsers = par("numUsers").intValue();
    
    // Rule 1: Root initialization
    if (nodeId == 0) {  // Assuming root has ID 0
        parentId = -1;
        level = 0;
        status = STABLE;
        payment = 0;
        beta = 0;
        calculateAlpha();
    } else {
        status = FAULTY;  // Start in faulty state
        level = INT_MAX;
    }
}

void SCMNode::handleMessage(cMessage *msg) {
    SCMControlMessage *ctrlMsg = dynamic_cast<SCMControlMessage*>(msg);
    if (!ctrlMsg) {
        delete msg;
        return;
    }
    
    switch (ctrlMsg->msgType) {
        case SCMControlMessage::ALPHA_UPDATE:
            // Handle alpha updates from children
            break;
            
        case SCMControlMessage::BETA_UPDATE:
            // Handle beta updates from parent
            break;
            
        case SCMControlMessage::FAULT_NOTIFY:
            // Handle fault notifications
            break;
    }
    
    delete ctrlMsg;
}

bool SCMNode::notLocallyConsistent() {
    // Implementation of Definition 2 from paper
    if (status == STABLE) {
        if (parentId == -1) {  // Root node
            return (level != 0) || (beta != 0) || (payment != 0);
        }
        
        // Check parent relationship
        cModule *parent = getParentModule();
        if (!parent) return true;
        
        // Check level consistency
        SCMNode *parentNode = check_and_cast<SCMNode*>(parent);
        if (level != parentNode->level + 1) return true;
        
        // Check beta calculation
        double expectedBeta = parentNode->beta + 
                            (par("linkCost").doubleValue() / parentNode->subtreeSize);
        if (std::abs(beta - expectedBeta) > 1e-6) return true;
    }
    return false;
}

// Other member function implementations to follow...