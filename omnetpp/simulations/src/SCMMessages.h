#ifndef SCMMESSAGES_H
#define SCMMESSAGES_H

#include <omnetpp.h>

class SCMControlMessage : public omnetpp::cMessage {
  public:
    enum MessageType {
        ALPHA_UPDATE,
        BETA_UPDATE,
        FAULT_NOTIFY
    };
    
  private:
    MessageType msgType;
    int senderId;
    double value;
    
  public:
    SCMControlMessage(const char *name=nullptr, short kind=0) : 
        cMessage(name, kind) {}
    
    // Getters and setters
    MessageType getMsgType() const { return msgType; }
    void setMsgType(MessageType t) { msgType = t; }
    int getSenderId() const { return senderId; }
    void setSenderId(int id) { senderId = id; }
    double getValue() const { return value; }
    void setValue(double v) { value = v; }
};

#endif // SCMMESSAGES_H