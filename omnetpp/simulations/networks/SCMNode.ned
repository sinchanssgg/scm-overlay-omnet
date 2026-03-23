
simple SCMNode {
    parameters:
        int id;
        int numUsers;
        double linkCost @default(1.0);
        string algorithmVariant @default("scm");

        @display("i=block/process");

    gates:
        inout port[];  // Bidirectional connections to neighbors
}
