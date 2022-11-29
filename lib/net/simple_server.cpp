//
// Created by ankit on 08.11.22.
//

#include "net_uhcustom.h"

class CustomServer : public uh::net::server_interface<CustomMsgTypes>
{
public:
    CustomServer(const uh::net::server_config& config)
        : uh::net::server_interface<CustomMsgTypes>(config)
    {

    }

protected:
    // custom implementation to reject a client connection
    bool onClientConnect(std::shared_ptr<uh::net::connection<CustomMsgTypes>> client) override {
        return true;
    }

    // Called when a client appears to have disconnected
    void onClientDisconnect(std::shared_ptr<uh::net::connection<CustomMsgTypes>> client) override {
        INFO << "removing client [" << client->getID() << "]\n";
    }

    // Called when a message arrives
    void onMessage(std::shared_ptr<uh::net::connection<CustomMsgTypes>> clientConnection, uh::net::message<CustomMsgTypes>& msg) override
    {
        switch (msg.header.id) {
            case CustomMsgTypes::Ping: {
                INFO << "[" << clientConnection->getID() << "]: received ping";
                clientConnection->send(msg);
                INFO << "sent pong";
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
    try
    {
        std::string userName = getlogin();

        uh::net::server_config config {
            .port = 6000,
            .tlsChain = "/home/" + userName +"/CLionProjects/3_Network_Communication/include/server.pem",
            .tlsKey = "/home/" + userName + "/CLionProjects/3_Network_Communication/include/server.key",
        };

        CustomServer server(config);
        server.start();

        while (true)
        {
            server.update();
        }

    }
    catch (const std::exception& e)
    {
        FATAL << e.what();
    }

    return 0;
}
