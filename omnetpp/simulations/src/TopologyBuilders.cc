#include <fstream>
#include <sstream>
#include <omnetpp.h>
#include <cmath>
#define OPENSSL_SUPPRESS_DEPRECATED
#include <openssl/rand.h>
#include "SCMNode.h"

using namespace omnetpp;

class CBTBuilder : public cModule {
  protected:
    virtual void initialize() override {
        int depth = par("depth");
        int numNodes = (int)pow(2, depth + 1) - 1;
        
        // Resize node vector if needed
        cModule *parent = getParentModule();
        parent->setSubmoduleVectorSize("node", numNodes);
        
        // Initialize all nodes with proper parameters
        for (int i = 0; i < numNodes; i++) {
            initializeNode(parent, i, depth);
        }
        
        // Build tree connections
        for (int i = 1; i < numNodes; i++) {
            int parentIdx = (i - 1) / 2;
            connectNodes(parent, parentIdx, i);
        }
        
        EV << "Complete Binary Tree built with " << numNodes << " nodes and depth " << depth << endl;
    }
    
    void initializeNode(cModule *network, int idx, int depth) {
        cModule *node = network->getSubmodule("node", idx);
        if (!node) return;
        
        // Set node parameters
        node->par("id") = idx;
        node->par("numUsers") = (int)uniform(1, 5);  // Random users per node
        node->par("linkCost") = (double)uniform(1, 10); // Random link cost
    }

    void connectNodes(cModule *network, int src, int dest) {
        cModule *srcMod = network->getSubmodule("node", src);
        cModule *destMod = network->getSubmodule("node", dest);
        
        if (!srcMod || !destMod) return;
        
        // Create bidirectional connection
        cGate *srcOut = srcMod->getOrCreateFirstUnconnectedGate("port$o", 0, false, true);
        cGate *destIn = destMod->getOrCreateFirstUnconnectedGate("port$i", 0, false, true);
        cGate *destOut = destMod->getOrCreateFirstUnconnectedGate("port$o", 0, false, true);
        cGate *srcIn = srcMod->getOrCreateFirstUnconnectedGate("port$i", 0, false, true);
        
        // Connect bidirectional channels
        cDatarateChannel *ch1 = cDatarateChannel::create("channel");
        ch1->setDelay(0.1); // 100ms delay
        srcOut->connectTo(destIn, ch1);
        
        cDatarateChannel *ch2 = cDatarateChannel::create("channel");
        ch2->setDelay(0.1); // 100ms delay
        destOut->connectTo(srcIn, ch2);
        
        EV << "Connected node " << src << " to node " << dest << endl;
    }
};
Define_Module(CBTBuilder);

class ERBuilder : public cModule {
  protected:
    virtual void initialize() override {
        std::string edgeFile = par("edgeFile");
        int numNodes = par("numNodes");
        
        cModule *network = getParentModule();
        network->setSubmoduleVectorSize("node", numNodes);
        
        // Initialize all nodes first
        for (int i = 0; i < numNodes; i++) {
            initializeNode(network, i);
        }
        
        // Read edges from file and create connections
        std::ifstream file(edgeFile.c_str());
        if (!file.is_open()) {
            throw cRuntimeError("Cannot open edge file: %s", edgeFile.c_str());
        }
        
        std::string line;
        int edgeCount = 0;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            int src, dest;
            iss >> src >> dest;
            
            if (src >= 0 && src < numNodes && dest >= 0 && dest < numNodes) {
                connectNodes(network, src, dest);
                edgeCount++;
            }
        }
        file.close();
        
        EV << "Erdos-Renyi graph built with " << numNodes << " nodes and " << edgeCount << " edges" << endl;
    }
    
    void initializeNode(cModule *network, int idx) {
        cModule *node = network->getSubmodule("node", idx);
        if (!node) return;
        
        // Set node parameters
        node->par("id") = idx;
        node->par("numUsers") = (int)uniform(1, 5);  // Random users per node
        node->par("linkCost") = (double)uniform(1, 10); // Random link cost
    }

    void connectNodes(cModule *network, int src, int dest) {
        cModule *srcMod = network->getSubmodule("node", src);
        cModule *destMod = network->getSubmodule("node", dest);

        if (!srcMod || !destMod) return;
        
        // Check if connection already exists to avoid duplicates
        if (areNodesConnected(srcMod, destMod)) {
            return;
        }
        
        // Create bidirectional connection
        cGate *srcOut = srcMod->getOrCreateFirstUnconnectedGate("port$o", 0, false, true);
        cGate *destIn = destMod->getOrCreateFirstUnconnectedGate("port$i", 0, false, true);
        cGate *destOut = destMod->getOrCreateFirstUnconnectedGate("port$o", 0, false, true);
        cGate *srcIn = srcMod->getOrCreateFirstUnconnectedGate("port$i", 0, false, true);
        
        // Connect bidirectional channels with random costs
        cDatarateChannel *ch1 = cDatarateChannel::create("channel");
        ch1->setDelay(0.1 + uniform(0, 0.05)); // 100-150ms delay with variation
        srcOut->connectTo(destIn, ch1);
        
        cDatarateChannel *ch2 = cDatarateChannel::create("channel");
        ch2->setDelay(0.1 + uniform(0, 0.05)); // 100-150ms delay with variation
        destOut->connectTo(srcIn, ch2);
        
        EV << "Connected node " << src << " to node " << dest << endl;
    }
    
    bool areNodesConnected(cModule *node1, cModule *node2) {
        // Check if nodes are already connected
        for (cModule::GateIterator it1(node1); !it1.end(); it1++) {
            cGate *gate1 = *it1;
            if (gate1->getType() == cGate::OUTPUT && gate1->isConnected()) {
                cGate *otherGate = gate1->getNextGate();
                if (otherGate && otherGate->getOwnerModule() == node2) {
                    return true;
                }
            }
        }
        return false;
    }
};
Define_Module(ERBuilder);

class TwitchBuilder : public cModule {
  protected:
    virtual void initialize() override {
        std::string edgeFile = par("edgeFile");
        int numNodes = par("numNodes");
        int rootId = par("rootId");
        
        cModule *network = getParentModule();
        network->setSubmoduleVectorSize("node", numNodes);
        
        // Initialize all nodes first
        for (int i = 0; i < numNodes; i++) {
            initializeNode(network, i, rootId);
        }
        
        // Read edges from Twitch dataset
        std::ifstream file(edgeFile.c_str());
        if (!file.is_open()) {
            throw cRuntimeError("Cannot open Twitch edge file: %s", edgeFile.c_str());
        }
        
        std::string line;
        int edgeCount = 0;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            std::istringstream iss(line);
            int src, dest;
            iss >> src >> dest;
            
            // Ensure nodes are within bounds for our simulation
            if (src >= 0 && src < numNodes && dest >= 0 && dest < numNodes) {
                connectNodes(network, src, dest);
                edgeCount++;
            }
        }
        file.close();
        
        EV << "Twitch graph built with " << numNodes << " nodes and " << edgeCount << " edges. Root: " << rootId << endl;
    }
    
    void initializeNode(cModule *network, int idx, int rootId) {
        cModule *node = network->getSubmodule("node", idx);
        if (!node) return;
        
        // Set node parameters
        node->par("id") = idx;
        node->par("numUsers") = (int)uniform(1, 5);  // Random users per node
        node->par("linkCost") = (double)uniform(1, 10); // Random link cost
    }

    void connectNodes(cModule *network, int src, int dest) {
        // Same implementation as ERBuilder::connectNodes
        cModule *srcMod = network->getSubmodule("node", src);
        cModule *destMod = network->getSubmodule("node", dest);
        
        if (!srcMod || !destMod) return;
        
        if (areNodesConnected(srcMod, destMod)) {
            return;
        }
        
        cGate *srcOut = srcMod->getOrCreateFirstUnconnectedGate("port$o", 0, false, true);
        cGate *destIn = destMod->getOrCreateFirstUnconnectedGate("port$i", 0, false, true);
        cGate *destOut = destMod->getOrCreateFirstUnconnectedGate("port$o", 0, false, true);
        cGate *srcIn = srcMod->getOrCreateFirstUnconnectedGate("port$i", 0, false, true);
        
        cDatarateChannel *ch1 = cDatarateChannel::create("channel");
        ch1->setDelay(0.1 + uniform(0, 0.02)); // 100-120ms delay
        srcOut->connectTo(destIn, ch1);
        
        cDatarateChannel *ch2 = cDatarateChannel::create("channel");
        ch2->setDelay(0.1 + uniform(0, 0.02)); // 100-120ms delay
        destOut->connectTo(srcIn, ch2);
    }
    
    bool areNodesConnected(cModule *node1, cModule *node2) {
        // Same implementation as ERBuilder::areNodesConnected
        for (cModule::GateIterator it1(node1); !it1.end(); it1++) {
            cGate *gate1 = *it1;
            if (gate1->getType() == cGate::OUTPUT && gate1->isConnected()) {
                cGate *otherGate = gate1->getNextGate();
                if (otherGate && otherGate->getOwnerModule() == node2) {
                    return true;
                }
            }
        }
        return false;
    }
};
Define_Module(TwitchBuilder);