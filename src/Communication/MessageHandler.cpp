/**
 * @file MessageHandler.cpp
 * @author paul
 * @date 01.06.19
 * @brief Implementation of the MessageHandler
 */

#include "MessageHandler.hpp"

namespace communication {
    MessageHandler::MessageHandler(const std::string &server, uint16_t port, util::Logging &log)
        : log{log}, socketClient{server, "/", port, "http-only"} {
        socketClient.receiveListener(
                std::bind(&MessageHandler::receiveEvent, this, std::placeholders::_1));
        socketClient.closeListener(closeListener);
    }

    void MessageHandler::send(messages::Message message) {
        nlohmann::json jsonMessage = message;
        try {
            socketClient.send(jsonMessage.dump(4));
        } catch (std::runtime_error &e) {
            log.error("Connection already closed!");
        }
    }

    void MessageHandler::receiveEvent(std::string msg) {
        if (!msg.empty()) {
            try {
                nlohmann::json json = nlohmann::json::parse(msg);
                auto message = json.get<messages::Message>();
                receiveListener(message);
            } catch (nlohmann::json::exception &e) {
                log.error("Got invalid json (or the f*cking lobby mod)!");
            } catch (std::runtime_error &e) {
                log.error("Got invalid json values!");
            }
        }
    }
}
