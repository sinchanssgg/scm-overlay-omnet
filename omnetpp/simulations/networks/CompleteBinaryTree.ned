
network CompleteBinaryTree
{
    parameters:
        int numNodes @default(31);
        @display("bgb=800,600");

    submodules:
        node[numNodes]: SCMNode {
            id = index;
            numUsers = default(intuniform(1,5));
            @display("i=misc/node");
        }

    connections allowunconnected:
        // Tree connections built programmatically by CBTBuilder
        // or via NED loops for parent-child pairs:
        for i=1..numNodes-1 {
            node[i].port++ <--> { delay = 100ms; } <--> node[int((i-1)/2)].port++;
        }
}
