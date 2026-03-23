#ifndef SCMFAULTINJECTOR_H
#define SCMFAULTINJECTOR_H

#include <omnetpp.h>
#include <vector>

class SCMFaultInjector : public omnetpp::cSimpleModule {
  public:
    enum FaultType {
        DISTANCE_TAMPER,
        BETA_MODIFICATION,
        PARENT_SWITCH
    };
    enum CampaignMode {
        PROBABILISTIC_PER_NODE,
        DETERMINISTIC_ONE_NODE_PER_DEPTH
    };
    
  private:
    FaultType faultType;
    CampaignMode campaignMode;
    int campaignSeed;
    int campaignRound;
    int maxCampaignDepth;
    int parentOffset;
    bool strictDepthCampaign;
    bool sendFaultNotify;

    std::vector<std::vector<int>> depthBuckets;

    int computeCbtDepthFromIndex(int nodeIndex) const;
    void buildDepthBuckets(int numNodes);
    std::vector<int> selectDeterministicTargets() const;
    void applyFaultToNode(class SCMNode *node, int numNodes);
    void notifyNodeFault(class SCMNode *node);
    void injectFault();
    
  protected:
    virtual void initialize() override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;
};

#endif // SCMFAULTINJECTOR_H
