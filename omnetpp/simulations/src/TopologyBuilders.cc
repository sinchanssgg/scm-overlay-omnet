#include <fstream>
#include <sstream>
#include <omnetpp.h>
#include "SCMNode.h"

using namespace omnetpp;

class CBTBuilder : public cModule {
  protected:
    virtual void initialize() override {
        int depth = par("depth");
        int numNodes = pow(2, depth + 1) - 1;
        
        // Resize node vector if needed
        cModule *parent = getParentModule();
        parent->setSubmoduleVectorSize("node", numNodes);
        
        // Build tree connections
        for (int i = 1; i < numNodes; i++) {
            int parentIdx = (i - 1) / 2;
            connectNodes(parent, parentIdx, i);
        }
    }
    
    void connectNodes(cModule *network, int src, int dest) {
        cModule *srcMod = network->getSubmodule("node", src);
        cModule *destMod = network->getSubmodule("node", dest);
        
        cGate *srcGate = srcMod->getOrCreateFirstUnconnectedGate("port", 0, false, true);
        cGate *destGate = destMod->getOrCreateFirstUnconnectedGate("port", 0, false, true);
        
        srcGate->connectTo(destGate);
        srcGate->getChannel()->par("delay") = 0.1; // 100ms
    }
};
Define_Module(CBTBuilder);

class ERBuilder : public cModule {
  protected:
    virtual void initialize() override {
        std::string edgeFile = par("edgeFile");
        std::ifstream file(edgeFile);
        std::string line;
        
        cModule *network = getParentModule();
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            int src, dest;
            iss >> src >> dest;
            
            connectNodes(network, src, dest);
        }
    }
    
    void connectNodes(cModule *network, int src, int dest) {
        // Same implementation as CBTBuilder
    }
};
Define_Module(ERBuilder);