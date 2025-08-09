#include <fstream>
#include <sstream>
#include <vector>
#include <omnetpp.h>
#include "SCMNode.h"

using namespace omnetpp;

class TwitchNetworkInitializer : public cModule
{
  protected:
    virtual void initialize() override {
        EV << "Initializing Twitch network from: " << par("edgeFilePath").stringValue() << endl;
        
        std::ifstream file(par("edgeFilePath").stringValue());
        if (!file.is_open()) {
            throw cRuntimeError("Cannot open edge file: %s", par("edgeFilePath").stringValue());
        }

        std::string line;
        int connectionsCreated = 0;
        
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            int source, target;
            char sep; // For comma or space separation
            
            if (!(iss >> source >> sep >> target)) {
                continue; // Skip malformed lines
            }

            // Validate node IDs
            if (source < 0 || source >= getParentModule()->par("numNodes") || 
                target < 0 || target >= getParentModule()->par("numNodes")) {
                EV_WARN << "Skipping invalid edge: " << source << "->" << target << endl;
                continue;
            }

            cModule *srcMod = getParentModule()->getSubmodule("node", source);
            cModule *tgtMod = getParentModule()->getSubmodule("node", target);
            
            if (!srcMod || !tgtMod) {
                EV_WARN << "Missing node for edge: " << source << "->" << target << endl;
                continue;
            }

            // Create connection
            cGate *srcGate = srcMod->getOrCreateFirstUnconnectedGate("port", 0, false, true);
            cGate *tgtGate = tgtMod->getOrCreateFirstUnconnectedGate("port", 0, false, true);
            
            srcGate->connectTo(tgtGate);
            cDatarateChannel *channel = cDatarateChannel::create("channel");
            channel->par("delay") = 0.1; // 100ms delay
            srcGate->setChannel(channel);
            
            connectionsCreated++;
            
            if (connectionsCreated % 100000 == 0) {
                EV << "Created " << connectionsCreated << " connections..." << endl;
            }
        }
        
        EV << "Twitch network initialization complete. Created " << connectionsCreated << " connections." << endl;
    }
};

Define_Module(TwitchNetworkInitializer);