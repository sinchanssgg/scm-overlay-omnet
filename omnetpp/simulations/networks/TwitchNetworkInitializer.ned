// @file TwitchNetworkInitializer.ned
// @brief Initializer module that builds Twitch network from edge file
// Author: Arannya Mukherjee <arannya@adhrith.ai>

simple TwitchNetworkInitializer {
    parameters:
        string edgeFilePath;

        @display("i=block/table,blue");
}
