
simple SCMFaultInjector {
    parameters:
        int faultType @default(0);           // 0=DISTANCE_TAMPER, 1=BETA_MODIFICATION, 2=PARENT_SWITCH
        double faultProbability @default(0);  // Per-node probability of fault injection
        double interval @unit(s) @default(10s);  // Time between injection rounds
        double initialDelay @unit(s) @default(5s); // Delay before first injection

        @display("i=block/cogwheel,red");
}
