package overlay;

network SCMNetwork
{
    parameters:
        int numNodes @default(10);
        double faultProbability @default(0.1);
        double faultInterval @default(10s);
        
        @display("bgb=800,600");
    
    submodules:
        faultInjector: SCMFaultInjector {
            faultProbability = faultProbability;
            interval = faultInterval;
        }
        
        node[numNodes]: SCMNode {
            id = index;
            numUsers = default(intuniform(1,5));
            @display("i=misc/node");
        }
    
    connections allowunconnected:
        // Create random connections (can replace with specific topology)
        for i=0..numNodes-2, for j=i+1..numNodes-1 {
            node[i].port++ <--> { delay = 100ms; } <--> node[j].port++ 
                if uniform(0,1) < 0.3; // 30% connection probability
        }
}