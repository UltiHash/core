//
// Created by ankit on 08.11.22.
//

#include <iostream>
#include "net_uhcustom.h"

/**
 * @brief CustomClient class inherits the client interface class and is used to connect to a server in an asynchronous fashion.
 *
 * The implementation details of the client-server communication is abstracted away by the client interface class.
 *
 */
class CustomClient : public uh::net::client_interface<CustomMsgTypes> {
public:
    /**
     * @brief pingServer is used to generate a custom ping message that encapsulates the time in order to see the round trip time.
     * @return void
     */
    void pingServer() {
        uh::net::message<CustomMsgTypes> pingMsg;
        pingMsg.header.id = CustomMsgTypes::Ping;

        // bad practice, but for now we know the system
        std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
        pingMsg << timeNow;
        std::cout << "Sending Ping" << std::endl;
        sendCus(pingMsg);
    }
};

template<> std::atomic<std::size_t> uh::net::connection<CustomMsgTypes>::s_runningConnections{0};

int main() {
    CustomClient cl;
    cl.connect("127.0.0.1", 6000);
    bool bQuit = false;
    cl.pingServer();
    while (!bQuit) {
        if (cl.isConnected()) {
            if (!cl.incoming().empty()) {
                auto msg = cl.incoming().pop_front().msg;
                switch (msg.header.id) {
                    case CustomMsgTypes::Ping: {
                        std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
                        std::chrono::system_clock::time_point timeThen;
                        msg >> timeThen;
                        std::cout << "Received Pong: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";
//                        bQuit = true;
                    }
                        break;
                }
            }
        } else {
            std::cout << "Disconnected from the server\n";
            bQuit = true;
        }
    }
    return 0;
}

