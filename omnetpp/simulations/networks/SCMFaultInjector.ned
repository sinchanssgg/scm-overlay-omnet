// @file SCMFaultInjector.ned
// @brief Fault injection module supporting distance, beta, and parent-switch faults
// Author: Arannya Mukherjee <arannya@adhrith.ai>
// Modified By: Sinchan Sengupta <sinchan.sengupta@univ-nantes.fr>

simple SCMFaultInjector {
    parameters:
        int faultType @default(0);           // 0=DISTANCE_TAMPER, 1=BETA_MODIFICATION, 2=PARENT_SWITCH
        double faultProbability @default(0);  // Per-node probability of fault injection
        double interval @unit(s) @default(10s);  // Time between injection rounds
        double initialDelay @unit(s) @default(5s); // Delay before first injection
        int campaignMode @default(0);  // 0=probabilistic per-node, 1=deterministic one-node-per-depth
        int campaignSeed @default(1337);
        int maxCampaignDepth @default(-1); // -1 means all depths
        int campaignTargetDepth @default(-1); // >=0: corrupt only at this exact depth; -1: use maxCampaignDepth range
        int parentOffset @default(1); // Used by deterministic parent-switch campaigns
        bool strictDepthCampaign @default(true);
        bool sendFaultNotify @default(true);

        @display("i=block/cogwheel,red");
}
