#ifndef SCMFAULTINJECTOR_H
#define SCMFAULTINJECTOR_H

#include <omnetpp.h>

class SCMFaultInjector : public omnetpp::cSimpleModule {
  public:
    enum FaultType {
        DISTANCE_TAMPER,
        BETA_MODIFICATION,
        PARENT_SWITCH
    };
    
  private:
    FaultType faultType;
    void injectFault();
    
  protected:
    virtual void initialize() override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;
};

#endif // SCMFAULTINJECTOR_H