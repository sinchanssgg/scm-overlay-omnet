// @file EdgeFileNetworkInitializer.ned
// @brief Generic initializer that builds network topology from edge-list file
// Author: Sinchan Sengupta <sinchan.sengupta@univ-nantes.fr>

simple EdgeFileNetworkInitializer {
    parameters:
        string edgeFilePath;

        @display("i=block/table,blue");
}
