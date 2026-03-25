/**
 * @file TwitchNetworkInitializer.cc
 * @brief Twitch network initializer — builds topology from edge file
 * Author: Sinchan Sengupta <sinchan.sengupta@univ-nantes.fr>
 * Modified By: Arannya Mukherjee <arannya@adhrith.ai>
 */
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <omnetpp.h>
#include "SCMNode.h"

using namespace omnetpp;

namespace {

static uint64_t edgeKey(int a, int b)
{
    int lo = std::min(a, b);
    int hi = std::max(a, b);
    return (static_cast<uint64_t>(static_cast<uint32_t>(lo)) << 32) |
           static_cast<uint32_t>(hi);
}

static bool parseEdgeLine(const std::string& line, int& source, int& target)
{
    if (line.empty() || line[0] == '#') {
        return false;
    }
    std::istringstream iss(line);
    if (!(iss >> source)) {
        return false;
    }
    if (iss.peek() == ',' || iss.peek() == ';') {
        iss.get();
    }
    if (!(iss >> target)) {
        return false;
    }
    return true;
}

} // namespace

class TwitchNetworkInitializer : public cSimpleModule
{
  protected:
    virtual void initialize() override {
        EV << "Initializing Twitch network from: " << par("edgeFilePath").stringValue() << endl;

        std::ifstream file(par("edgeFilePath").stringValue());
        if (!file.is_open()) {
            throw cRuntimeError("Cannot open edge file: %s", par("edgeFilePath").stringValue());
        }

        int numNodes = (int)getParentModule()->par("numNodes");

        std::string line;
        int connectionsCreated = 0;
        int parsed = 0;
        int skippedOutOfBounds = 0;
        int skippedDuplicates = 0;
        std::unordered_set<uint64_t> seen;

        while (std::getline(file, line)) {
            int source = -1;
            int target = -1;
            if (!parseEdgeLine(line, source, target)) {
                continue;
            }
            parsed++;

            // Validate node IDs
            if (source == target ||
                source < 0 || source >= numNodes ||
                target < 0 || target >= numNodes) {
                skippedOutOfBounds++;
                continue;
            }

            uint64_t key = edgeKey(source, target);
            if (!seen.insert(key).second) {
                skippedDuplicates++;
                continue;
            }

            cModule *srcMod = getParentModule()->getSubmodule("node", source);
            cModule *tgtMod = getParentModule()->getSubmodule("node", target);

            if (!srcMod || !tgtMod) {
                throw cRuntimeError("Missing node module for edge (%d,%d)", source, target);
            }

            // Create bidirectional connection with channels
            cGate *srcOut = srcMod->getOrCreateFirstUnconnectedGate("port$o", 0, false, true);
            cGate *tgtIn = tgtMod->getOrCreateFirstUnconnectedGate("port$i", 0, false, true);
            cGate *tgtOut = tgtMod->getOrCreateFirstUnconnectedGate("port$o", 0, false, true);
            cGate *srcIn = srcMod->getOrCreateFirstUnconnectedGate("port$i", 0, false, true);

            cDelayChannel *ch1 = cDelayChannel::create("channel");
            ch1->setDelay(0.1);
            srcOut->connectTo(tgtIn, ch1);
            ch1->callInitialize();

            cDelayChannel *ch2 = cDelayChannel::create("channel");
            ch2->setDelay(0.1);
            tgtOut->connectTo(srcIn, ch2);
            ch2->callInitialize();

            connectionsCreated++;
        }

        if (connectionsCreated == 0) {
            throw cRuntimeError("No valid edges were created from edge file: %s", par("edgeFilePath").stringValue());
        }

        EV << "Twitch network initialization complete. Created " << connectionsCreated
           << " undirected edges (" << parsed << " parsed, "
           << skippedDuplicates << " duplicates, "
           << skippedOutOfBounds << " out-of-bounds)." << endl;
    }
};

Define_Module(TwitchNetworkInitializer);
