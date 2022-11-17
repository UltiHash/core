//
// Created by ankit on 08.11.22.
//

#include <iostream>
#include "../include/net_uhcustom.h"

class CustomServer : public uh::net::server_interface<CustomMsgTypes>
{
public:
    CustomServer(uint16_t nPort) : uh::net::server_interface<CustomMsgTypes>(nPort)
    {

    }

protected:
    // custom implementation to reject a client connection
    bool onClientConnect(std::shared_ptr<uh::net::connection<CustomMsgTypes>> client) override {
        return true;
    }

    // Called when a client appears to have disconnected
    void onClientDisconnect(std::shared_ptr<uh::net::connection<CustomMsgTypes>> client) override {
        std::cout << "Removing client [" << client->getID() << "]\n";
    }

    // Called when a message arrives
    void onMessage(std::shared_ptr<uh::net::connection<CustomMsgTypes>> clientConnection, uh::net::message<CustomMsgTypes>& msg) override
    {
        switch (msg.header.id) {
            case CustomMsgTypes::Ping: {
                std::cout << "[" << clientConnection->getID() << "]: Received Ping.\n";
                clientConnection->send(msg);
                std::cout << "[SERVER]: Sent Pong.\n";
            }
            break;
            case CustomMsgTypes::ServerAccept:
                break;
            case CustomMsgTypes::ServerDeny:
                break;
        }
    }
};

template<> std::atomic<std::size_t> uh::net::connection<CustomMsgTypes>::s_runningConnections{0};

int main()
{
    CustomServer server(6000);
    server.start();

    while (1)
    {
        server.update();
    }
    return 0;
}