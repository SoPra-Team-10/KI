/**
 * @file ArgumentParser.hpp
 * @author paul
 * @date 01.05.19
 * @brief Declaration of the ArgumentParser class
 */

#ifndef SERVER_ARGUMENTPARSER_HPP
#define SERVER_ARGUMENTPARSER_HPP

#include <string>
#include <optional>

namespace util {
    /**
     * Implements a basic argument parser according to the standard using GNU-Getopt
     */
    class ArgumentParser {
    public:
        /**
         * CTor, parses the arguments and fills all fields. If --help is found printHelp() is called and the
         * program gets terminated. If no argument is passed the help is printed as well.
         * @throws std::runtime_error if any of the required flags doesn't exist or contains an invalid value
         * @param argc the number of arguments, as passed into main
         * @param argv an array of all arguments, as passed into main
         */
        ArgumentParser(int argc, char *argv[]);

        std::string getAddress() const;

        std::string getConfigPath() const;

        std::string getLobbyName() const;

        std::string getUName() const;

        std::string getPw() const;

        int getPort() const;

        int getDifficulty() const;

        int getVerbosity() const;

        /**
         * Prints the help message, gets called by the CTor if the -h or --help flag is set.
         */
        static void printHelp();
    private:
        std::string address;
        std::string configPath;
        std::string lobbyName;
        std::string uName;
        std::string pw;
        int port;
        int difficulty;
        int verbosity;
    };
}

#endif //SERVER_ARGUMENTPARSER_HPP
