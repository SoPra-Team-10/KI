/**
 * @file Communicator.hpp
 * @author paul
 * @date 02.06.19
 * @brief Declares the Communicator class
 */

#ifndef KI_COMMUNICATOR_HPP
#define KI_COMMUNICATOR_HPP

#include <string>
#include <queue>
#include <SopraUtil/Logging.hpp>
#include <SopraMessages/Message.hpp>
#include <SopraMessages/TeamConfig.hpp>
#include <Game/Game.hpp>
#include <SopraUtil/Timer.h>
#include "MessageHandler.hpp"

namespace communication {
    /**
     * This module is responsible for sending and receiving messages according to the protocol
     * defined in the standard.
     */
    class Communicator {
    public:
        /**
         * CTor. Constructs a Game and a MessageHandler
         * @param lobbyName the name of the lobby to join
         * @param userName the username to use
         * @param password the password to use
         * @param difficulty the difficulty of the AI
         * @param teamConfig the teamConfig to use
         * @param server the server to use for the WebSocketClient
         * @param port the port to use for the WebSocketClient
         * @param log a log object for logging
         * @see Game, MessageHandler
         */
        Communicator(const std::string &lobbyName, const std::string &userName,
                const std::string &password, unsigned int difficulty,
                const messages::request::TeamConfig &teamConfig,
                const std::string &server, uint16_t port, util::Logging &log);

    private:
        void onMessageReceive(const messages::Message& message);
        void send(const messages::Payload &payload);

        template <typename T>
        void onPayloadReceive(const T &payload);

        void onClose();

        void reconnectRunner();

        std::optional<MessageHandler> messageHandler;
        std::string server;
        uint16_t port;
        std::string lobbyName, userName, password;
        Game game;
        messages::request::TeamConfig teamConfig;
        util::Logging &log;
        std::atomic_bool paused = false;
        util::Timer timer;
        std::condition_variable cvMainToWorker;
        std::mutex pauseMutex;
        std::mutex updateMutex;
        std::thread worker;
        std::atomic_bool isConnected = true;
        std::future<void> reconnectThread;
        bool teamConfigSent;
    };
}

#endif //KI_COMMUNICATOR_HPP
