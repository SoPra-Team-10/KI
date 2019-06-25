/**
 * @file Communicator.cpp
 * @author paul
 * @date 02.06.19
 * @brief Defines the communicator class
 */

#include "Communicator.hpp"

namespace communication {
    Communicator::Communicator(const std::string &lobbyName, const std::string &userName,
                                const std::string &password,
                                unsigned int difficulty, const messages::request::TeamConfig &teamConfig,
                                const std::string &server, uint16_t port, util::Logging &log)
            : messageHandler{server, port, log}, game{difficulty, teamConfig}, log{log} {
        messageHandler.receiveListener(
                std::bind(&Communicator::onMessageReceive, this, std::placeholders::_1));

        send(messages::request::JoinRequest{lobbyName, userName, password, true});
        log.info("Send JoinRequest");
        send(teamConfig);
        log.info("Send TeamConfig");
    }

    template <>
    void Communicator::onPayloadReceive<messages::broadcast::MatchStart>(
            const messages::broadcast::MatchStart &payload) {
        log.info("Got MatchStart");
        send(game.getTeamFormation(payload));
        log.info("Send TeamFormation");
    }

    template <>
    __attribute__((noreturn)) void Communicator::onPayloadReceive<messages::broadcast::MatchFinish>(
            const messages::broadcast::MatchFinish &) {
        log.info("Got MatchFinish, exiting");
        //@TODO maybe print some more information (who won...)
        std::exit(0);
    }

    template <>
    void Communicator::onPayloadReceive<messages::broadcast::Snapshot>(
            const messages::broadcast::Snapshot &payload) {
        log.info("Got Snapshot");
        game.onSnapshot(payload);
    }

    template <>
    void Communicator::onPayloadReceive<messages::broadcast::Next>(const messages::broadcast::Next &next) {
        log.info("Got Next request");
        auto request = game.getNextAction(next);
        if(request.has_value()) {
            log.info("Requesting Action");
            send(*request);
        } else {
            log.info("Next ignored");
        }
    }

    template <>
    void Communicator::onPayloadReceive<messages::unicast::PrivateDebug>(
            const messages::unicast::PrivateDebug &privateDebug) {
        log.warn("Got private debug:");
        log.warn(privateDebug.getInformation());
    }

    template<typename T>
    void Communicator::onPayloadReceive(const T&) {
        log.warn("Got unhandled message:");
        log.warn(T::getName());
    }


    void Communicator::onMessageReceive(const messages::Message& message) {
        std::visit([this](const auto &payload){
            this->onPayloadReceive(payload);
        }, message.getPayload());
    }

    void Communicator::send(const messages::Payload &payload) {
        messageHandler.send(messages::Message{payload});
    }
}