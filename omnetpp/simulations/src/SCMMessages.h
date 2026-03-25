/**
 * @file SCMMessages.h
 * @brief SCM control message types for inter-node communication
 * Author: Sinchan Sengupta <sinchan.sengupta@univ-nantes.fr>
 * Modified By: Arannya Mukherjee <arannya@adhrith.ai>
 */
#ifndef SCMMESSAGES_H
#define SCMMESSAGES_H

#include <omnetpp.h>

class SCMControlMessage : public omnetpp::cMessage {
  public:
    // Canonical enum name used across SCMNode.h and SCMNode.cc
    enum MsgType {
        ALPHA_UPDATE,
        BETA_UPDATE,
        FAULT_NOTIFY,
        PROOF_REQUEST,
        PROOF_RESPONSE
    };

    // Keep old name as alias so existing code using MessageType still compiles
    typedef MsgType MessageType;

  private:
    MsgType msgType;
    int senderId;
    double value;

  public:
    SCMControlMessage(const char *name=nullptr, short kind=0)
        : cMessage(name, kind), msgType(ALPHA_UPDATE), senderId(-1), value(0) {}

    // OMNeT++ requires dup() for message cloning
    virtual SCMControlMessage *dup() const override {
        return new SCMControlMessage(*this);
    }

    // Getters and setters
    MsgType getMsgType() const { return msgType; }
    void setMsgType(MsgType t) { msgType = t; }
    int getSenderId() const { return senderId; }
    void setSenderId(int id) { senderId = id; }
    double getValue() const { return value; }
    void setValue(double v) { value = v; }
};

#endif // SCMMESSAGES_H
