/**
 * @file MessageHandler.hpp
 * @author paul
 * @date 01.06.19
 * @brief Declaration of the MessageHandler class
 */

#ifndef KI_MESSAGEHANDLER_HPP
#define KI_MESSAGEHANDLER_HPP

#include <SopraNetwork/WebSocketClient.hpp>
#include <SopraMessages/Message.hpp>

#include <Util/Logging.hpp>

namespace communication {
    class MessageHandler {
    public:
        MessageHandler(const std::string &server, uint16_t port, util::Logging &log);
        void send(messages::Message message);
        const util::Listener<messages::Message> receiveListener;
        const util::Listener<> closeListener;
    private:
        void receiveEvent(std::string msg);
        util::Logging &log;
        network::WebSocketClient socketClient;
    };
}

#endif //KI_MESSAGEHANDLER_HPP
