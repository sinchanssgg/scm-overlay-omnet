#include <omnetpp.h>

class SCMNode : public cSimpleModule {
protected:
    virtual void initialize() override {
        EV << "Hello from " << getFullPath() << endl;
    }
};
Define_Module(SCMNode);