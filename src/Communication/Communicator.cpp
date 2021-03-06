/**
 * @file Communicator.cpp
 * @author paul
 * @date 02.06.19
 * @brief Defines the communicator class
 */

#include "Communicator.hpp"

namespace communication {
    constexpr auto RECONNECT_INTERVAL = 1000;

    Communicator::Communicator(const std::string &lobbyName, const std::string &userName,
                                const std::string &password,
                                unsigned int difficulty, const messages::request::TeamConfig &teamConfig,
                                const std::string &server, uint16_t port, util::Logging &log)
            : messageHandler{}, server{server}, port{port}, lobbyName{lobbyName}, userName{userName}, password{password},
                game{difficulty, teamConfig, log}, teamConfig{teamConfig}, log{log}, teamConfigSent{false} {
        messageHandler.emplace(server, port, log);
        messageHandler->receiveListener(
                std::bind(&Communicator::onMessageReceive, this, std::placeholders::_1));
        messageHandler->closeListener(std::bind(&Communicator::onClose, this));
        send(messages::request::JoinRequest{lobbyName, userName, password, true});
        log.info("Send JoinRequest");
    }

    template <>
    void Communicator::onPayloadReceive(const messages::unicast::JoinResponse &) {
        if (!teamConfigSent) {
            log.info("Got Join Response");
            send(teamConfig);
            log.info("Send TeamConfig");
            teamConfigSent = true;
        }
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
            const messages::broadcast::MatchFinish &matchFinish) {
        log.info("Got MatchFinish, exiting");
        log.info("Winner: " + matchFinish.getWinnerUserName());
        std::exit(0);
    }

    template <>
    void Communicator::onPayloadReceive<messages::broadcast::Snapshot>(
            const messages::broadcast::Snapshot &payload) {
        if(updateMutex.try_lock()){
            log.info("Got Snapshot, updating");
            game.onSnapshot(payload);
            updateMutex.unlock();
        }
    }

    template <>
    void Communicator::onPayloadReceive<messages::broadcast::Next>(const messages::broadcast::Next &next) {
        using namespace communication::messages;
        log.info("Got Next request");
        auto computeNextAsync = [this, next](){
            std::optional<request::DeltaRequest> request;

            {
                std::lock_guard lock(updateMutex);
                request = game.getNextAction(next, timer);
            }

            if(!request.has_value()){
                return;
            }

            if(paused){
                std::unique_lock<std::mutex> lock(pauseMutex);
                cvMainToWorker.wait(lock, [this] { return !static_cast<bool>(paused); });
            }

            log.info("Sending ->");
            send(*request);
            log.debug("Type sent: " + types::toString(request->getDeltaType()));
            if(request->getActiveEntity().has_value()){
                log.debug("ID sent: " + types::toString(request->getActiveEntity().value()));
            }
        };

        if(worker.joinable()){
            worker.join();
        }

        log.debug("Starting worker...");
        worker = std::thread(computeNextAsync);
    }

    template <>
    void Communicator::onPayloadReceive<messages::broadcast::PauseResponse>(const messages::broadcast::PauseResponse &pauseResponse){
        {
            std::lock_guard<std::mutex> lock(pauseMutex);
            log.info("Pause response received");
            paused = pauseResponse.isPause();
        }

        cvMainToWorker.notify_all();
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
        isConnected = true;
        std::visit([this](const auto &payload){
            this->onPayloadReceive(payload);
        }, message.getPayload());
    }

    void Communicator::send(const messages::Payload &payload) {
        if (isConnected) {
            messageHandler.value().send(messages::Message{payload});
        }
    }

    void Communicator::onClose() {
        log.error("Closed");
        isConnected = false;
        reconnectThread = std::async(std::launch::async, std::bind(&Communicator::reconnectRunner, this));
    }

    void Communicator::reconnectRunner() {
        while (!isConnected) {
            messageHandler.reset();
            log.info("Trying reconnect");
            messageHandler.emplace(server, port, log);
            messageHandler->receiveListener(
                    std::bind(&Communicator::onMessageReceive, this, std::placeholders::_1));
            std::this_thread::sleep_for(std::chrono::milliseconds{RECONNECT_INTERVAL});
        }

        messageHandler->closeListener(std::bind(&Communicator::onClose, this));
        log.info("Reconnect successful");

        send(messages::request::JoinRequest{lobbyName, userName, password, true});
        log.info("Send JoinRequest");
    }
}
