#include <iostream>
#include <getopt.h>

#define PORT_DEFAULT 4488
#define DIFFICULTY_DEFAULT 1
#define VERBOSE_DEFAULT 0

/**
 * Prints the usage of cli parameters to stdout
 */
void showHelp(){
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

int main(int argc, char** argv) {
    std::string address;
    std::string configPath;
    std::string lobbyName;
    std::string uName;
    std::string pw;
    int port = PORT_DEFAULT;
    int difficulty = DIFFICULTY_DEFAULT;
    int verbosity = VERBOSE_DEFAULT;

    struct option options[] = {
            {"address", required_argument, nullptr, 'a'},
            {"team", required_argument, nullptr, 't'},
            {"lobby", required_argument, nullptr, 'l'},
            {"username", required_argument, nullptr, 'u'},
            {"password", required_argument, nullptr, 'k'},
            {"port", required_argument, nullptr, 'p'},
            {"difficulty", required_argument, nullptr, 'd'},
            {"verbosity", required_argument, nullptr, 'v'},
    };

    int c = 0;
    int optionIndex = -1;

    auto parse = [options](int* target, char option){
        try {
            *target = std::stoi(optarg);
        }catch (std::invalid_argument &e){
            std::cerr << "Invalid argument for option '" << option << "', integer required!\n" << e.what() << std::endl;
        }
    };

    while((c = getopt_long(argc, argv, "a:t:l:u:p:k:d:v:h", options, &optionIndex)) != -1){
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
                parse(&port, c);
                break;
            case 'd':
                parse(&difficulty, c);
                break;
            case 'v':
                parse(&verbosity, c);
                break;
            case 'h':
                showHelp();
                break;
            case '?':
                std::cerr << "Error parsing cli parameters" << std::endl;
                return -1;
            default:
                std::cerr << "Error parsing cli parameters" << std::endl;
                return -1;
        }
    }

    if(address.empty()){
        std::cerr << "Missing mandatory option 'address'. Please specify the address to the game server." << std::endl;
        return -1;
    }

    if(configPath.empty()){
        std::cerr << "Missing mandatory option 'team'. Please specify the path to a valid team configuration file." << std::endl;
        return -1;
    }

    if(lobbyName.empty()){
        std::cerr << "Missing mandatory option 'lobby'. Please specify the name of a lobby." << std::endl;
        return -1;
    }
    return 0;
}