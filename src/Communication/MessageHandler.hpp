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

#include <SopraUtil/Logging.hpp>

namespace communication {
    /**
     * The Message Handler is responsible for (de-)serializing all messages.
     */
    class MessageHandler {
    public:
        /**
         * CTor. Constructs a WebSocketClient
         * @param server the address (either IP or URL) of the server
         * @param port the port of the server
         * @param log a log object used for logging
         */
        MessageHandler(const std::string &server, uint16_t port, util::Logging &log);

        /**
         * Send a message to the server
         * @param message the message to send
         */
        void send(messages::Message message);

        /**
         * Event that gets called when a new message is received
         */
        const util::Listener<messages::Message> receiveListener;

        /**
         * Event that is called when the connection gets closed
         */
        const util::Listener<> closeListener;
    private:
        void receiveEvent(std::string msg);
        util::Logging &log;
        network::WebSocketClient socketClient;
    };
}

#endif //KI_MESSAGEHANDLER_HPP
