// @file SimpleNetwork.ned
// @brief Minimal two-node network for basic SCM testing
// Author: Sinchan Sengupta <sinchan.sengupta@univ-nantes.fr>
// Modified By: Arannya Mukherjee <arannya@adhrith.ai>

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
