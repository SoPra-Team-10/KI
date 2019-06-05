/**
 * @file Communicator.hpp
 * @author paul
 * @date 02.06.19
 * @brief Communicator @TODO
 */

#ifndef KI_COMMUNICATOR_HPP
#define KI_COMMUNICATOR_HPP

#include <string>
#include <SopraUtil/Logging.hpp>
#include <SopraMessages/Message.hpp>
#include <SopraMessages/TeamConfig.hpp>
#include <Game/Game.hpp>
#include "MessageHandler.hpp"

namespace communication {
    class Communicator {
    public:
        Communicator(const std::string &lobbyName, const std::string &userName,
                const std::string &password, unsigned int difficulty,
                const messages::request::TeamConfig &teamConfig,
                const std::string &server, uint16_t port, util::Logging &log);

    private:
        void onMessageReceive(messages::Message message);
        void send(const messages::Payload &payload);

        template <typename T>
        void onPayloadReceive(const T &payload);

        MessageHandler messageHandler;
        Game game;
        util::Logging &log;
    };
}

#endif //KI_COMMUNICATOR_HPP
