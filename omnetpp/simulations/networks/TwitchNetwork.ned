package overlay;

import "SCMNode.ned";
import "SCMFaultInjector.ned";

network TwitchNetwork
{
    parameters:
        string edgeFilePath @default("/home/sengupta-s/Downloads/large_twitch_edges.csv");
        int numNodes @default(168114); // From Twitch dataset
        @display("bgb=1000,800");
    
    submodules:
        initializer: TwitchNetworkInitializer {
            edgeFilePath = edgeFilePath;
        }
        
        faultInjector: SCMFaultInjector {
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