#include <omnetpp.h>

class SCMNode : public cSimpleModule {
protected:
    virtual void initialize() override {
        EV << "Initializing " << getFullPath() << endl;
    }
};
Define_Module(SCMNode);