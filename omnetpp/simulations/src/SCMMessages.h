#ifndef SCMMESSAGES_H
#define SCMMESSAGES_H

#include <omnetpp.h>

// Message types for SCM protocol
class SCMControlMessage : public omnetpp::cMessage {
  public:
    enum MessageType {
        ALPHA_UPDATE,  // For subtree size propagation
        BETA_UPDATE,   // For cost share propagation
        FAULT_NOTIFY   // For fault propagation
    };
    
    MessageType msgType;
    int senderId;
    double value;  // Can carry alpha/beta values
    
    SCMControlMessage(const char *name=nullptr, short kind=0) : cMessage(name, kind) {}
};

#endif // SCMMESSAGES_H