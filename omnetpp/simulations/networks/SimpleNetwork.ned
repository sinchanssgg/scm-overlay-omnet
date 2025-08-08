simple SCMNode {
    gates:
        input in;
        output out;
}

network SimpleNetwork {
    submodules:
        node1: SCMNode;
        node2: SCMNode;
    connections:
        node1.out --> node2.in;
}