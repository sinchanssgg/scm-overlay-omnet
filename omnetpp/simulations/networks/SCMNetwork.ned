import "SCMNode.ned";
import "SCMFaultInjector.ned";

network SCMNetwork {
    parameters:
        int numNodes;
        @display("bgb=800,600");
    
    submodules:
        faultInjector: SCMFaultInjector {
            @display("p=50,50");
        }
        
        node[numNodes]: SCMNode {
            numUsers = default(uniform(1,5));
            @display("i=block/routing");
        }
    
    connections allowunconnected:
        // Define your network connections here
        // Example: node[0].port++ <--> node[1].port++;
}