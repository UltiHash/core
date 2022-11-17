//
// Created by ankit on 26.10.22.
//

#ifndef INC_3_NETWORK_COMMUNICATION_NET_CLIENT_H
#define INC_3_NETWORK_COMMUNICATION_NET_CLIENT_H

#include "net_common.h"
#include "net_connection.h"

/**
 * @brief Namespace for UltiHash's custom networking implementations.
 */
namespace uh::net {

    /**
     * @brief This is the access point for other application to talk to the server.
     * @tparam T Data type of the element.
     *
     * This client interface object sets up asio and abstracts all the asio related asynchronous implementations.
     */
    template <typename T>
    class client_interface {
    public:

        client_interface() {
            //initialize the socket with the io context
        }

        virtual ~client_interface() {
            //disconnect from the server to destroy the client object
            disconnect();
        }

        //connect to the server with hostname/ip-address and port
        bool connect(const std::string& host, const uint16_t port) {
            try {
                // resolve hostname/ip-address into physical addresses
                boost::asio::ip::tcp::resolver resolver(m_context);
                auto m_endpoints = resolver.resolve(host, std::to_string(port));

                //create connection
                m_connection = std::make_unique<connection<T>>(connection<T>::owner::client, m_context, m_SSLcontext, m_qMessagesIn);

                //use connection object to connect to the server
                //TODO: make client connection asynchronous since client can also utilize some time on calculating something instead of just waiting
                try {
                    boost::asio::connect(m_connection->sslSocket().lowest_layer(), m_endpoints);
                    m_connection->sslSocket().handshake(boost::asio::ssl::stream_base::client);
                    std::cout << "[CLIENT] Handshake successful!\n";
                    m_connection->connectToServer();
                } catch(boost::system::error_code &err) {
                    std::cout << "Connection to Server Failed. " << err.message() << "\n";
                }
                //asio context thread
                thrContext = std::thread([this]() { m_context.run(); });
            } catch(std::exception& e) {
                std::cerr << "Client Exception: " << e.what() << "\n";
                return false;
            }
            return true;
        }

        //disconnect from the server
        void disconnect() {
            // disconnect only if connection exists
            if (isConnected()) {
                // logic of disconnect is inside the connection object
                m_connection->disconnect();
            }
            // stop the context and the thread where it runs
            m_context.stop();
            if (thrContext.joinable())
                thrContext.join();
            //destroy the connection object
            m_connection.release();
        }

        //check if connection exists
        bool isConnected() {
            if (m_connection) {
                return m_connection->isConnected();
            } else {
                return false;
            }
        }

        //getter for the queue structure
        tsqueue<owned_message<T>>& incoming() {
            return m_qMessagesIn;
        }

        // Send message to server
        void sendCus(const message<T>& msg)
        {
            if (isConnected())
                m_connection->send(msg);
        }

    protected:
        //asio context in its own thread to handle data transfer
        boost::asio::io_context m_context;
        boost::asio::ssl::context m_SSLcontext{boost::asio::ssl::context::tlsv13};
        std::thread thrContext;
        //an instance of the connection object for handling data transfer
        std::unique_ptr<connection<T>> m_connection;

    private:
        //incoming queue of messages from the server
        tsqueue<owned_message<T>> m_qMessagesIn;
    };
}

#endif //INC_3_NETWORK_COMMUNICATION_NET_CLIENT_H