#include <Util/ArgumentParser.hpp>
#include <iostream>
#include <Util/Logging.hpp>
#include <Communication/MessageHandler.hpp>
#include <filesystem>
#include <fstream>

int main(int argc, char** argv) {
    std::string address;
    std::string teamConfigPath;
    std::string lobbyName;
    std::string uName;
    std::string pw;
    uint16_t port;
//    unsigned int difficulty;
    unsigned int verbosity;

    try {
        util::ArgumentParser argumentParser{argc, argv};
        address = argumentParser.getAddress();
        teamConfigPath = argumentParser.getConfigPath();
        lobbyName = argumentParser.getLobbyName();
        uName = argumentParser.getUName();
        pw = argumentParser.getPw();
        port = argumentParser.getPort();
       // difficulty = argumentParser.getDifficulty();
        verbosity = argumentParser.getVerbosity();
    } catch (std::invalid_argument &e) {
        std::cerr << e.what() << std::endl;
        std::exit(1);
    }

    if (!std::filesystem::exists(teamConfigPath)) {
        std::cerr << "Team config file doesn't exist" << std::endl;
        std::exit(1);
    }

    communication::messages::request::TeamConfig teamConfig;
    try {
        nlohmann::json json;
        std::ifstream ifstream{teamConfigPath};
        ifstream >> json;
        teamConfig = json.get<communication::messages::request::TeamConfig>();
    } catch (nlohmann::json::exception &e) {
        std::cerr << e.what() << std::endl;
        std::exit(1);
    } catch (std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
        std::exit(1);
    }

    util::Logging log{std::cout, verbosity};
    communication::MessageHandler messageHandler{address, port, log};

    return 0;
}