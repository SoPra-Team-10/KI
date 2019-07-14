/**
 * @file ArgumentParser.hpp
 * @author paul
 * @date 01.05.19
 * @brief Declaration of the ArgumentParser class
 */

#ifndef KI_ARGUMENTPARSER_HPP
#define KI_ARGUMENTPARSER_HPP

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

        /**
         * Get the address
         * @return the value given to the address flag
         */
        std::string getAddress() const;

        /**
         * Get the config path
         * @return the value given to the teamConfig flag
         */
        std::string getConfigPath() const;

        /**
         * Return the lobby
         * @return the value given to the lobby flag or "hogwarts"
         */
        std::string getLobbyName() const;

        /**
         * Return the username
         * @return the value given to the username flag or "Team10Ki"
         */
        std::string getUName() const;

        /**
         * Return the password
         * @return the value given to the password flag or "password"
         */
        std::string getPw() const;

        /**
         * Return the port
         * @return the value given to the port flag or 4488
         */
        int getPort() const;

        /**
         * Return the difficulty
         * @return the value given to the difficulty flag or 0
         */
        int getDifficulty() const;

        /**
         * Return the verbosity
         * @return the value given to the verbosity flag or 0
         */
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
        uint port{};
        unsigned int difficulty{};
        unsigned int verbosity{};
    };
}

#endif //KI_ARGUMENTPARSER_HPP
