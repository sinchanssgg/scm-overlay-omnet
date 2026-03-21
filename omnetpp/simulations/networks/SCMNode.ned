package overlay;

simple SCMNode {
    parameters:
        int id;
        int numUsers;
        double linkCost @default(1.0);

        @display("i=block/process");

    gates:
        inout port[];  // Bidirectional connections to neighbors
}
