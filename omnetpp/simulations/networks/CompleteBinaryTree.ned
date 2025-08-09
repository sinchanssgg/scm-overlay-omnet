package overlay;

import "SCMNode.ned";

network CompleteBinaryTree
{
    parameters:
        int depth @default(4); // Creates 2^(depth+1)-1 nodes
        @display("bgb=800,600");
    
    submodules:
        node[2^(depth+1)-1]: SCMNode {
            id = index;
            numUsers = default(intuniform(1,5));
            @display("i=misc/node");
        }
    
    connections allowunconnected:
        // Build complete binary tree structure
        for i=0..size-1 {
            int parent = (i-1)/2;
            if (parent >= 0) {
                node[i].port++ <--> { 
                    delay = 100ms; 
                    @display("ls=green,2");
                } <--> node[parent].port++;
            }
        }
    
    // Position nodes for visualization
    visual:
        for i=0..size-1 {
            int level = log2(i+1);
            int posInLevel = i - (2^level - 1);
            int x = 400 + (posInLevel - (2^level-1)/2.0) * 150;
            int y = 100 + level * 100;
            node[i].setDisplayString("p=$x,$y");
        }
}