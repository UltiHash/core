#pragma once

#include "command_factory.h"
#include "cors/module.h"
#include "http/request_factory.h"
#include "policy/module.h"

namespace uh::cluster::ep {

class handler_factory;

class handler : public protocol_handler {
public:
    handler(boost::asio::ip::tcp::socket&& socket, handler_factory& factory)
        : m_socket{std::move(socket)},
          m_factory{factory} {
        LOG_INFO() << "session started: " << m_socket.remote_endpoint();
    }
    ~handler() {
        LOG_INFO() << "session ended: " << m_socket.remote_endpoint();
    }

    void cancel() override {
        if (m_socket.is_open()) {
            try {
                LOG_DEBUG() << "socket canceled";
                m_socket.cancel();
            } catch (const boost::system::system_error& e) {
                LOG_ERROR() << "cancel failed: " << e.what();
            }
        } else {
            LOG_ERROR() << "socket is not open, cancel skipped.";
        }
    }
    coro<void> run() override;

private:
    boost::asio::ip::tcp::socket m_socket;
    handler_factory& m_factory;

    coro<http::response> handle_request(http::raw_request& rawreq,
                                        const std::string& id);
};

class handler_factory : public protocol_handler_factory {
public:
    explicit handler_factory(command_factory&& comm_factory,
                             http::request_factory&& factory,
                             std::unique_ptr<policy::module> policy,
                             std::unique_ptr<cors::module> cors);

    std::unique_ptr<protocol_handler>
    create_handler(boost::asio::ip::tcp::socket&& s) {
        return std::make_unique<handler>(std::move(s), *this);
    }

private:
    friend class handler;
    command_factory m_command_factory;
    http::request_factory m_request_factory;
    std::unique_ptr<policy::module> m_policy;
    std::unique_ptr<cors::module> m_cors;
};
} // end namespace uh::cluster::ep
