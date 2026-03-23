
network CompleteBinaryTree
{
    parameters:
        int numNodes @default(31);
        string edgeFilePath @default("");
        double faultProbability @default(0);
        double faultInterval @unit(s) @default(10s);
        @display("bgb=800,600");

    submodules:
        initializer: EdgeFileNetworkInitializer {
            edgeFilePath = parent.edgeFilePath;
            @display("p=50,80");
        }

        faultInjector: SCMFaultInjector {
            faultProbability = parent.faultProbability;
            interval = parent.faultInterval;
            @display("p=50,50");
        }

        node[numNodes]: SCMNode {
            id = index;
            numUsers = default(intuniform(1,5));
            @display("i=misc/node");
        }

    connections allowunconnected:
        // Connections are created by EdgeFileNetworkInitializer from edgeFilePath.
}
