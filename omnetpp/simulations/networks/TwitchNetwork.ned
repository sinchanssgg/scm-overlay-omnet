// @file TwitchNetwork.ned
// @brief Twitch social graph network topology for large-scale SCM simulation
// Author: Sinchan Sengupta <sinchan.sengupta@univ-nantes.fr>
// Modified By: Arannya Mukherjee <arannya@adhrith.ai>

network TwitchNetwork
{
    parameters:
        string edgeFilePath;  // Set via omnetpp.ini or --result-dir
        int numNodes @default(168114); // From Twitch dataset
        double faultProbability @default(0);
        double faultInterval @unit(s) @default(10s);
        @display("bgb=1000,800");

    submodules:
        initializer: TwitchNetworkInitializer {
            edgeFilePath = parent.edgeFilePath;
        }

        faultInjector: SCMFaultInjector {
            faultProbability = parent.faultProbability;
            interval = parent.faultInterval;
            @display("p=50,50");
        }
        
        node[numNodes]: SCMNode {
            id = index;
            numUsers = default(intuniform(1,5));
            @display("i=misc/node_s");
        }
    
    connections allowunconnected:
        // Connections will be created programmatically
}
