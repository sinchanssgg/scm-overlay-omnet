#ifndef SCMNODE_H
#define SCMNODE_H

#include <omnetpp.h>
#include "SCMMessages.h"

class SCMNode : public omnetpp::cSimpleModule {
  private:
    // Node state variables (from Algorithm 1)
    int nodeId;
    int parentId;
    int level;
    enum Status { STABLE, FAULTY, RECOVERING } status;
    double payment;
    int subtreeSize;
    double beta;
    int numUsers;
    
    // Utility functions
    bool notLocallyConsistent();
    bool lostStableSupport();
    void calculateAlpha();
    void calculateBeta();
    
  protected:
    virtual void initialize() override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;
    virtual void refreshDisplay() const override;
};

#endif // SCMNODE_H