package overlay;

import "SCMNode.ned";
import "SCMFaultInjector.ned";

network ErdosRenyi
{
    parameters:
        int numNodes @default(20);
        double connectionProbability @default(0.3);
        @display("bgb=800,600");
    
    submodules:
        faultInjector: SCMFaultInjector {
            @display("p=50,50");
        }
        
        node[numNodes]: SCMNode {
            id = index;
            numUsers = default(intuniform(1,5));
            x = uniform(0,800);
            y = uniform(0,600);
            @display("p=$x,$y;i=misc/node");
        }
    
    connections allowunconnected:
        // Random connections with given probability
        for i=0..numNodes-2, for j=i+1..numNodes-1 {
            node[i].port++ <--> { 
                delay = 100ms; 
                @display("ls=blue,1");
            } <--> node[j].port++ 
                if uniform(0,1) < connectionProbability;
        }
}