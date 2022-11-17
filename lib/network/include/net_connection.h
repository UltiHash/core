//
// Created by ankit on 26.10.22.
//

#ifndef INC_3_NETWORK_COMMUNICATION_NET_CONNECTION_H
#define INC_3_NETWORK_COMMUNICATION_NET_CONNECTION_H

#include "net_common.h"
#include "net_message.h"
#include "net_queue_ts.h"

/**
 * @brief Namespace for custom networking implementations.
 */
namespace uh::net{
    /**
     * @brief A connection object for handling the connection between a server and a client.
     * @tparam T Data type of the element.
     *
     * The connection object handles both the incoming as well as the outgoing messages to/from a specific remote connection by using the thread-safe dequeue structures. It is also responsible
     * for storing the remote socket connection and the shared asio context.
     */
    template<typename T>
    class connection : public std::enable_shared_from_this<connection<T>> {
    public:
        enum class owner{
            server,
            client
        };
        connection(owner parent, boost::asio::io_context &asioContext, boost::asio::ssl::context &context, tsqueue <owned_message<T>> &qIn)
                : m_asioContext(asioContext), m_socket(asioContext,context), m_qMessagesIn(qIn) {
            m_nOwnerType = parent;
            s_runningConnections++;
        }

        virtual ~connection() {
            s_runningConnections--;
        }

        // QUESTION: IS THIS A GOOD PRACTICE? It's not a proper abstraction of the socket implementation.
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket>& sslSocket() {
            return m_socket;
        }

        boost::asio::ssl::stream<boost::asio::ip::tcp::socket>::lowest_layer_type &socket()
        {
            return m_socket.lowest_layer();
        }

    public:
        static std::size_t runningConnections() { return s_runningConnections; }

        void disconnect() {
            if (isConnected()) {
                boost::asio::post(m_asioContext,[this](){m_socket.lowest_layer().close();});
            }
        }

        [[nodiscard]] bool isConnected() const {
            return m_socket.lowest_layer().is_open();
        }

        [[nodiscard]] uint64_t getID() const {
            return id;
        }

        void send(const message<T>& msg) {
            boost::asio::post(m_asioContext, [this, msg]() {
                // only assign writeHeader work to the asio if the queue is empty
                bool bwritingMessages = m_qMessagesOut.empty();
                m_qMessagesOut.push_back(msg);
                if (bwritingMessages) {
                    writeHeader();
                }
            });
        }

        // for server connection object
        void connectToClient(uint64_t uid=0) {
            if (m_nOwnerType==owner::server) {
                if (m_socket.lowest_layer().is_open()) {
                    id=uid;
                    readHeader();
                }
            }
        }

        void connectToServer() {
            if (m_nOwnerType==owner::client) {
                if (m_socket.lowest_layer().is_open()) {
                    readHeader();
                }
            }
        }

    protected:
        // each connection has a unique socket to a remote, and this socket has to be an encrypted tls connection
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket> m_socket;

        //asio context which is shared with the asio instance
        boost::asio::io_context& m_asioContext;

        //queue of outgoing messages to be sent to remote connection; handled by the asio context and the connection
        tsqueue<message<T>> m_qMessagesOut;
        //queue of incoming messages to be handled by a server or a client
        tsqueue<owned_message<T>>& m_qMessagesIn;

        // temporary message header storage
        message<T> m_tempMessage;

        owner m_nOwnerType = owner::server;
        uint64_t id=0;

        static std::atomic<std::size_t> s_runningConnections;

    private:
        // ASYNCHRONOUS - Assign header reading work to the asio context upon successful connection
        void readHeader() {
            // framework specific: can create a new thread which only handles the message read/write of different connections, we just do it in the thread of asio context
            boost::asio::async_read(m_socket, boost::asio::buffer(&m_tempMessage.header, sizeof(message_header<T>)),
                             [this](std::error_code ec, std::size_t length){
                if (!ec) {
//                    std::cout << "Message header from the incoming queue read..." << std::endl;
                    if (m_tempMessage.header.size > 0) {
                        // allocate memory for message body
                        m_tempMessage.body.resize(m_tempMessage.header.size);
                        readBody();
                    } else {
                        addToIncomingMessageQueue();
                    }
                } else {
                    std::cout << "[" << id << "]: " << "Reading the header of the message failed. " << ec.message() << "\n";
                    m_socket.lowest_layer().close();
                }
            });
        }

        void readBody(){
            boost::asio::async_read(m_socket,boost::asio::buffer(m_tempMessage.body.data(), m_tempMessage.body.size()),
                             [this](std::error_code ec, std::size_t length) {
                if (!ec) {
//                    std::cout << "Message body from the incoming queue read..." << std::endl;
                    addToIncomingMessageQueue();
                } else {
                    std::cout << "[" << id << "]: " << "Reading the body of the message failed. " << ec.message() << "\n";
                    m_socket.lowest_layer().close();
                }
            });
        }

        void writeHeader() {
            boost::asio::async_write(m_socket,boost::asio::buffer(&m_qMessagesOut.front().header,sizeof(message_header<T>)),
                            [this](std::error_code ec, std::size_t length) {
                if (!ec) {
//                    std::cout << "Message header from the asio queue sent..\n";
                    if (m_qMessagesOut.front().body.size() > 0 ) {
                        writeBody();
                    } else {
                        m_qMessagesOut.pop_front();
                        if (!m_qMessagesOut.empty()) {
                            writeHeader();
                        }
                    }
                } else {
                    std::cout << "[" << id << "]: " << "Writing the header of the message failed. " << ec.message() << "\n";
                    m_socket.lowest_layer().close();
                }
            });
        }

        void writeBody() {
            boost::asio::async_write(m_socket,boost::asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),
                              [this](std::error_code ec, std::size_t length) {
              if (!ec) {
//                  std::cout << "Message body from the asio queue sent..\n";
                  m_qMessagesOut.pop_front();
                  if (!m_qMessagesOut.empty()) {
                      writeHeader();
                  }
              } else {
                  std::cout << "[" << id << "]: " << "Writing the body of the message failed. " << ec.message() << "\n";
                  m_socket.lowest_layer().close();
              }
          });
        }

        /**
         * @brief addToIncomingMessageQueue is used to add incoming messages into the server's or client's ts-dequeue structure
         */
        void addToIncomingMessageQueue() {
            if (m_nOwnerType==owner::server) {
                m_qMessagesIn.push_back({this->shared_from_this(), m_tempMessage});
            } else {
                m_qMessagesIn.push_back({nullptr, m_tempMessage});
            }
            readHeader();
        }
    };
}

#endif