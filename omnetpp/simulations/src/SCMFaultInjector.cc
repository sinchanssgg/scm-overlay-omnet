#include "SCMFaultInjector.h"
#include "SCMNode.h"
#include <cstdlib>

Define_Module(SCMFaultInjector);

void SCMFaultInjector::initialize() {
    // Schedule first fault injection
    scheduleAt(omnetpp::simTime() + par("initialDelay").doubleValue(), 
               new omnetpp::cMessage("InjectFault"));
}

void SCMFaultInjector::handleMessage(cMessage *msg) {
    if (strcmp(msg->getName(), "InjectFault") == 0) {
        injectFault();
        scheduleAt(omnetpp::simTime() + par("interval").doubleValue(), msg);
    } else {
        delete msg;
    }
}

void SCMFaultInjector::injectFault() {
    int faultType = par("faultType").intValue();
    cModule *network = getParentModule();
    
    for (int i = 0; i < network->getSubmoduleVectorSize("node"); i++) {
        if (uniform(0, 1) < par("faultProbability").doubleValue()) {
            SCMNode *node = check_and_cast<SCMNode*>(
                network->getSubmodule("node", i));
            
            switch (faultType) {
                case DISTANCE_TAMPER:
                    node->bubble("Distance tampered!");
                    // Implement distance tampering logic
                    break;
                    
                case BETA_MODIFICATION:
                    node->bubble("Beta modified!");
                    // Implement beta modification logic
                    break;
            }
        }
    }
}