#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <omnetpp.h>

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

class EdgeFileNetworkInitializer : public cSimpleModule
{
  protected:
    virtual void initialize() override
    {
        const char *edgePathParam = par("edgeFilePath").stringValue();
        std::string edgePath = edgePathParam ? edgePathParam : "";
        if (edgePath.empty()) {
            throw cRuntimeError("edgeFilePath is empty");
        }

        std::ifstream file(edgePath.c_str());
        if (!file.is_open()) {
            throw cRuntimeError("Cannot open edge file: %s", edgePath.c_str());
        }

        cModule *network = getParentModule();
        if (!network) {
            throw cRuntimeError("Initializer has no parent network module");
        }
        int numNodes = network->par("numNodes").intValue();
        if (numNodes <= 0) {
            throw cRuntimeError("numNodes must be positive (got %d)", numNodes);
        }

        std::unordered_set<uint64_t> seen;
        std::string line;
        int created = 0;
        int parsed = 0;
        int skippedOutOfBounds = 0;
        int skippedDuplicates = 0;

        while (std::getline(file, line)) {
            int source = -1;
            int target = -1;
            if (!parseEdgeLine(line, source, target)) {
                continue;
            }
            parsed++;

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

            cModule *srcMod = network->getSubmodule("node", source);
            cModule *dstMod = network->getSubmodule("node", target);
            if (!srcMod || !dstMod) {
                throw cRuntimeError("Missing node module for edge (%d,%d)", source, target);
            }

            cGate *srcOut = srcMod->getOrCreateFirstUnconnectedGate("port$o", 0, false, true);
            cGate *dstIn = dstMod->getOrCreateFirstUnconnectedGate("port$i", 0, false, true);
            cGate *dstOut = dstMod->getOrCreateFirstUnconnectedGate("port$o", 0, false, true);
            cGate *srcIn = srcMod->getOrCreateFirstUnconnectedGate("port$i", 0, false, true);

            cDelayChannel *ch1 = cDelayChannel::create("channel");
            ch1->setDelay(0.1);
            srcOut->connectTo(dstIn, ch1);
            ch1->callInitialize();

            cDelayChannel *ch2 = cDelayChannel::create("channel");
            ch2->setDelay(0.1);
            dstOut->connectTo(srcIn, ch2);
            ch2->callInitialize();

            created++;
        }

        if (created == 0) {
            throw cRuntimeError("No valid edges were created from edge file: %s", edgePath.c_str());
        }

        EV << "EdgeFileNetworkInitializer: created " << created
           << " undirected edges (" << parsed << " parsed, "
           << skippedDuplicates << " duplicates, "
           << skippedOutOfBounds << " out-of-bounds) from "
           << edgePath << endl;
    }
};

Define_Module(EdgeFileNetworkInitializer);
