#ifndef SCMNODE_H
#define SCMNODE_H

#include <omnetpp.h>
#include "SCMMessages.h"

class SCMNode : public omnetpp::cSimpleModule {
  private:
    // Node state variables
    int id;
    int parentId;
    int level;
    enum Status { STABLE, FAULTY, RECOVERING } status;
    double payment;
    int subtreeSize;
    double beta;
    int numUsers;
    double linkCost;
    simsignal_t stabilizationTimeSignal;
    double lastFaultTime;
    
    // Helper methods
    bool notLocallyConsistent();
    bool lostStableSupport();
    bool allChildrenRecovering();
    bool existsBetterParent();
    void calculateAlpha();
    void calculateBeta();
    void notifyChildren();
    void rejoinTree();
    
  protected:
    virtual void initialize() override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;
    virtual void refreshDisplay() const override;
    
  public:
    // Getters for fault injection
    int getId() const { return id; }
    int getParentId() const { return parentId; }
    int getLevel() const { return level; }
    double getBeta() const { return beta; }
    
    // Setters for fault injection
    void setLevel(int lvl) { level = lvl; }
    void setBeta(double b) { beta = b; }
    void setParentId(int pid) { parentId = pid; }
};

#endif // SCMNODE_H