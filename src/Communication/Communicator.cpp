/**
 * @file Communicator.cpp
 * @author paul
 * @date 02.06.19
 * @brief Communicator @TODO
 */

#include "Communicator.hpp"

namespace communication {
    Communicator::Communicator(const std::string &lobbyName, const std::string &userName,
                                const std::string &password,
                                unsigned int difficulty, const messages::request::TeamConfig &teamConfig,
                                const std::string &server, uint16_t port, util::Logging &log)
            : messageHandler{server, port, log}, game{difficulty} {
        send(messages::request::JoinRequest{lobbyName, userName, password, true});
        send(teamConfig);
        send(game.getTeamFormation());
    }


    template<typename T>
    void Communicator::onPayloadReceive(const T &payload) {

    }

    void Communicator::onMessageReceive(messages::Message &message) {
        std::visit([this](const auto &payload){
            this->onPayloadReceive(payload);
        }, message.getPayload());
    }

    void Communicator::send(const messages::Payload &payload) {
        messageHandler.send(messages::Message{payload});
    }
}