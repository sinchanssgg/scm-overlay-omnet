package overlay;

import "SCMNode.ned";

network SimpleNetwork {
    submodules:
        node1: SCMNode {
            id = 0;
            numUsers = 1;
        }
        node2: SCMNode {
            id = 1;
            numUsers = 1;
        }
    connections:
        node1.port++ <--> { delay = 100ms; } <--> node2.port++;
}
