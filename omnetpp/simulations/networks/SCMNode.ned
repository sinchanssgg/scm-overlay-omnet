simple SCMNode {
    parameters:
        int numUsers;  // Number of users at this node
        double linkCost @default(1.0);  // Transmission cost
        
        @display("i=block/process");
    
    gates:
        input in[];  // Input gates
        output out[]; // Output gates
}