//
// Created by ankit on 26.10.22.
//

#ifndef INC_3_NETWORK_COMMUNICATION_NET_SERVER_H
#define INC_3_NETWORK_COMMUNICATION_NET_SERVER_H

#include "net_common.h"
#include "net_queue_ts.h"
#include "net_message.h"
#include "net_connection.h"

/**
 * @brief Namespace for UltiHash's custom networking implementations.
 */
namespace uh::net {
    /**
     * @brief An access point for a custom server applications that abstracts away all the asynchronous asio implementations.
     * @tparam T Data Type of the parameter.
     *
     */
    template<typename T>
    class server_interface {
    public:
        explicit server_interface(uint16_t port) : m_asioAcceptor(m_asioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {
            // provide server certificate for the handshake process
            std::string userName = getlogin();
            m_SSLcontext.use_certificate_chain_file("/home/" + userName +"/CLionProjects/3_Network_Communication/include/server.pem");
            m_SSLcontext.use_private_key_file("/home/" + userName + "/CLionProjects/3_Network_Communication/include/server.key", boost::asio::ssl::context::pem);
        }

        virtual ~server_interface() {
            stop();
        }

        bool start() {
            try {
                // priming the asio context with a work before running it in a thread
                waitForClientConnection();
                m_threadContext = std::jthread([this]() { m_asioContext.run(); });
            } catch (std::exception &e) {
                std::cerr << "[SERVER] Exception: " << e.what() << "\n";
                return false;
            }

            std::cout << "[SERVER] started...\n";
            return true;
        }

        void stop() {
            // request the context to close
            m_asioContext.stop();
            // see if the context thread is still active or not and join it
            if (m_threadContext.joinable()) {m_threadContext.join();}

            std::cout << "[SERVER] stopped.\n";
        }

        // ASYNCHRONOUS - Instructs asio to listen for the connections
        void waitForClientConnection() {
            std::shared_ptr<connection<T>> newconn = std::make_shared<connection<T>>(connection<T>::owner::server, m_asioContext, m_SSLcontext, m_qMessageIn);
            m_asioAcceptor.async_accept(newconn->socket(), [this, newconn](const std::error_code ec) {
                if (!ec) {
                    // custom logic implementation to deny the connection
                    if (onClientConnect(newconn)) {
                        std::cout << "[SERVER] New Connection Request: " << newconn->socket().remote_endpoint() << "\n";
                        newconn->sslSocket().async_handshake(boost::asio::ssl::stream_base::server, [this, newconn](const std::error_code ec) {
                            if (!ec) {
                                std::cout << "[Server] Handshake Successful!\n";
                                m_deqConnections.push_back(std::move(newconn));
                                m_deqConnections.back()->connectToClient(nIDCounter++);
                                std::cout << "["<< m_deqConnections.back()->getID() << "] Connection Approved.\n";
                            } else {
                                std::cout << "[Server] Handshake Failed! " << ec.message() << "\n";
                                newconn->socket().close();
                            }
                        });
                    } else {
                        std::cout << "[SERVER] Connection Denied.\n";
                    }
                } else {
                    std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
                }
                // prime the asio context to listen for more connections
                waitForClientConnection();
            });
        }

        // send specific message to a specific client connection, remove the connection from the ts-dequeue is socket is closed
        void sendMessageToClient(std::shared_ptr<connection<T>> client, const message<T>& msg) {
            if (client && client->isConnected()) {
                client->send(msg);
            } else {
                onClientDisconnect(client);
                client.reset();
                m_deqConnections.erase(std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end());
            }
        }

        // called by the server when it wants to process the stored messages queue
        void update(size_t nMaxMessages = -1) {
            size_t nMessageCount=0;
            while(nMessageCount < nMaxMessages && !m_qMessageIn.empty()) {
                auto owned_msg = m_qMessageIn.pop_front();
                //pass to the message handler
                onMessage(owned_msg.remote, owned_msg.msg);
                nMessageCount++;
            }
        }

    protected:
        // called when a client connects, veto the connection by return false
        virtual bool onClientConnect(std::shared_ptr<connection<T>> client) {
            return false;
        }

        // called when a client disconnects, veto the connection by return false
        virtual void onClientDisconnect(std::shared_ptr<connection<T>> client) {
        }

        // called when the message arrives
        virtual void onMessage(std::shared_ptr<connection<T>> client, message<T>& msg) {
            std::cout << "Should be overriden!" << std::endl;
        }

    protected:
        // thread safe queue of incoming messages that the server uses
        tsqueue<owned_message<T>> m_qMessageIn;

        // Important Note: Connections are removed from the dequeue of connections only when server tries to send messages to the clients
        // container for storing all the client connections
        tsqueue<std::shared_ptr<connection<T>>> m_deqConnections;

        // prepare the thread for io context to run
        boost::asio::io_context m_asioContext;
        boost::asio::ssl::context m_SSLcontext{boost::asio::ssl::context::tlsv13};
        std::jthread m_threadContext;

        // object for accepting new client connections
        boost::asio::ip::tcp::acceptor m_asioAcceptor;

        // clients need to be identified uniquely in the server
        uint64_t nIDCounter = 10000;
    };
}

#endif //INC_3_NETWORK_COMMUNICATION_NET_SERVER_H