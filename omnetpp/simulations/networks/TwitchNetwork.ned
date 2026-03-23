
network TwitchNetwork
{
    parameters:
        string edgeFilePath;  // Set via omnetpp.ini or --result-dir
        int numNodes @default(168114); // From Twitch dataset
        @display("bgb=1000,800");
    
    submodules:
        initializer: TwitchNetworkInitializer {
            edgeFilePath = parent.edgeFilePath;
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
