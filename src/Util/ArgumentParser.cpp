/**
 * @file ArgumentParser.cpp
 * @author paul
 * @date 01.05.19
 * @brief Definition of the ArgumentParser class
 */

#include "ArgumentParser.hpp"
#include <getopt.h>
#include <iostream>

#define PORT_DEFAULT 4488
#define DIFFICULTY_DEFAULT 1
#define VERBOSE_DEFAULT 0

namespace util {
    ArgumentParser::ArgumentParser(int argc, char **argv) {
        if (argc <= 1) {
            printHelp();
            std::exit(1);
        }

        struct option longopts[] = {
                {"address", required_argument, nullptr, 'a'},
                {"team", required_argument, nullptr, 't'},
                {"lobby", required_argument, nullptr, 'l'},
                {"username", required_argument, nullptr, 'u'},
                {"password", required_argument, nullptr, 'k'},
                {"port", required_argument, nullptr, 'p'},
                {"difficulty", required_argument, nullptr, 'd'},
                {"verbosity", required_argument, nullptr, 'v'},
                {}
        };

        int c = 0;
        int optionIndex = -1;

        auto parse = [](int* target, const std::string &optName){
            try {
                *target = std::stoi(optarg);
            }catch (std::invalid_argument &e){
                std::cerr << "Invalid argument for option '" << optName << "', integer required!\n" << e.what() << std::endl;
            }
        };

        int initialPort = PORT_DEFAULT, initialDifficulty = DIFFICULTY_DEFAULT, initialVerbosity = VERBOSE_DEFAULT;
        while((c = getopt_long(argc, argv, "a:t:l:u:p:k:d:v:h", longopts, &optionIndex)) != -1){
            std::string optionName;
            if(optionIndex == -1){
                optionName = static_cast<char>(c);
            } else {
                optionName = longopts[optionIndex].name;
            }

            switch(c){
                case 0:
                    //Flag was set by getopt, do nothing
                    continue;
                case 'a':
                    address = optarg;
                    break;
                case 't':
                    configPath = optarg;
                    break;
                case 'l':
                    lobbyName = optarg;
                    break;
                case 'u':
                    uName = optarg;
                    break;
                case 'k':
                    pw = optarg;
                    break;
                case 'p':
                    parse(&initialPort, optionName);
                    break;
                case 'd':
                    parse(&initialDifficulty, optionName);
                    break;
                case 'v':
                    parse(&initialVerbosity, optionName);
                    break;
                case 'h':
                    printHelp();
                    std::exit(1);
                default:
                    throw std::invalid_argument{"Error parsing cli parameters"};
            }

            optionIndex = -1;
        }

        if(address.empty()){
            throw std::invalid_argument{
                "Missing mandatory option 'address'. Please specify the address to the game server."};
        }

        if(configPath.empty()){
            throw std::invalid_argument{
                "Missing mandatory option 'team'. Please specify the path to a valid team configuration file."};
        }

        if(lobbyName.empty()){
            throw std::invalid_argument{"Missing mandatory option 'lobby'. Please specify the name of a lobby."};
        }

        if (initialPort < 0 || initialPort > 65535) {
            throw std::invalid_argument{"Port is not a valid port"};
        }

        if (initialDifficulty < 0) {
            throw std::invalid_argument{"Difficulty needs to be none negative"};
        }

        if (initialVerbosity < 0) {
            throw std::invalid_argument{"Verbosity needs to be none negative"};
        }

        port = static_cast<uint16_t>(initialPort);
        difficulty = static_cast<unsigned int>(initialDifficulty);
        verbosity = static_cast<unsigned int>(initialVerbosity);
    }

    void ArgumentParser::printHelp() {
        std::cout << "Usage:\n\n"
                  << "Mandatory options:\n"
                  << "\t -a/--address: The address to the game server\n"
                  << "\t -t/--team: Path to the team configuration file\n"
                  << "\t -l/--lobby: Name of the desired lobby\n\n"
                  << "Optional options:\n"
                  << "\t -u/--username: Username of the AI player\n"
                  << "\t -k/--password: Password of the AI player\n"
                  << "\t -p/--port: Port to connect to\n"
                  << "\t -d/--difficulty: Strength of the AI Player. Choose between 0 (maximum difficulty) and 2\n"
                  << "\t -v/--verbosity: Displays additional information (0 = none, 1 = error level, 2 = warn level, 3 = info level, 4 = debug level)"
                  << std::endl;
    }

    std::string ArgumentParser::getAddress() const {
        return address;
    }

    std::string ArgumentParser::getConfigPath() const {
        return configPath;
    }

    std::string ArgumentParser::getLobbyName() const {
        return lobbyName;
    }

    std::string ArgumentParser::getUName() const {
        return uName;
    }

    std::string ArgumentParser::getPw() const {
        return pw;
    }

    int ArgumentParser::getPort() const {
        return port;
    }

    int ArgumentParser::getDifficulty() const {
        return difficulty;
    }

    int ArgumentParser::getVerbosity() const {
        return verbosity;
    }
}
