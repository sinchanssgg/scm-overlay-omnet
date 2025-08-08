#ifndef SCMFAULTINJECTOR_H
#define SCMFAULTINJECTOR_H

#include <omnetpp.h>

class SCMFaultInjector : public omnetpp::cSimpleModule {
  private:
    enum FaultType {
        NONE,
        DISTANCE_TAMPER,
        BETA_MODIFICATION
    };
    
    void injectFault();
    
  protected:
    virtual void initialize() override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;
};

#endif // SCMFAULTINJECTOR_H