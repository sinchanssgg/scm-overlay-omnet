
network ErdosRenyi
{
    parameters:
        int numNodes @default(20);
        double connectionProbability @default(0.3);
        double faultProbability @default(0);
        double faultInterval @unit(s) @default(10s);
        @display("bgb=800,600");
    
    submodules:
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
        // Random connections with given probability
        for i=0..numNodes-2, for j=i+1..numNodes-1 {
            node[i].port++ <--> { 
                delay = 100ms; 
                @display("ls=blue,1");
            } <--> node[j].port++ 
                if uniform(0,1) < connectionProbability;
        }
}
