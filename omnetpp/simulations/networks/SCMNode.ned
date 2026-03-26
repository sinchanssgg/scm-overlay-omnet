// @file SCMNode.ned
// @brief SCM overlay node module with configurable algorithm variant
// Author: Sinchan Sengupta <sinchan.sengupta@univ-nantes.fr>
// Modified By: Arannya Mukherjee <arannya@adhrith.ai>

simple SCMNode {
    parameters:
        int id;
        int numUsers;
        double linkCost @default(1.0);
        string algorithmVariant @default("scm");
        bool globalRoundMode @default(false);
        @signal[nodeStableTime](type=double);
        @statistic[nodeStableTime](source=nodeStableTime; record=vector);

        @display("i=block/process");

    gates:
        input faultIn;  // Dedicated gate for fault-injector direct notifications
        inout port[];  // Bidirectional connections to neighbors
}
