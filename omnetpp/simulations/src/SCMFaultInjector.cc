#include "SCMFaultInjector.h"
#include "SCMNode.h"
#include "SCMMessages.h"

Define_Module(SCMFaultInjector);

void SCMFaultInjector::initialize()
{
    faultType = (FaultType)par("faultType").intValue();
    scheduleAt(simTime() + par("initialDelay").doubleValue(), 
              new cMessage("InjectFault"));
}

void SCMFaultInjector::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage() && strcmp(msg->getName(), "InjectFault") == 0) {
        injectFault();
        scheduleAt(simTime() + par("interval").doubleValue(), msg);
    } else {
        delete msg;
    }
}

void SCMFaultInjector::injectFault()
{
    cModule *network = getParentModule();
    int numNodes = network->par("numNodes");
    
    for (int i = 0; i < numNodes; i++) {
        if (uniform(0, 1) < par("faultProbability").doubleValue()) {
            SCMNode *node = check_and_cast<SCMNode*>(
                network->getSubmodule("node", i));
            
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
                        node->setParentId((node->getParentId() + 1) % numNodes);
                        node->bubble("PARENT SWITCHED");
                    }
                    break;
            }
            
            // Notify neighbors about the fault
            SCMControlMessage *faultMsg = new SCMControlMessage("FaultNotify");
            faultMsg->setMsgType(SCMControlMessage::FAULT_NOTIFY);
            send(faultMsg, "out", i);
        }
    }
}