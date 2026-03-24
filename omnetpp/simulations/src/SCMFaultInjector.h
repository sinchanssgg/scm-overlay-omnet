#ifndef SCMFAULTINJECTOR_H
#define SCMFAULTINJECTOR_H

#include <omnetpp.h>
#include <string>
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
    int campaignExactLevel;
    int parentOffset;
    bool strictDepthCampaign;
    bool sendFaultNotify;
    bool oneShotCampaign;
    bool enableMetricSampling;
    double sampleDelay;
    int sampleSerial;
    std::string resultDir;
    std::vector<std::string> sampleRows;

    std::vector<std::vector<int>> depthBuckets;

    int computeCbtDepthFromIndex(int nodeIndex) const;
    void buildDepthBuckets(int numNodes, bool allowCbtFallback);
    std::vector<int> selectDeterministicTargets() const;
    void applyFaultToNode(class SCMNode *node, int numNodes, int corruptionLevel, double baselineBetaAvg, double baselinePaymentAvg);
    void notifyNodeFault(class SCMNode *node);
    void scheduleMetricSample(int nodeId, int level, double baselineBeta, double baselinePayment);
    void handleMetricSample(omnetpp::cMessage *msg);
    void injectFault();
    
  protected:
    virtual void initialize() override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;
    virtual void finish() override;
};

#endif // SCMFAULTINJECTOR_H
