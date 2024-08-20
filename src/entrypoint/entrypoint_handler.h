#ifndef ENTRYPOINT_HANDLER_H
#define ENTRYPOINT_HANDLER_H

#include "command_factory.h"
#include "common/debug/class_name.h"
#include "http/command_exception.h"
#include "http/request_factory.h"

namespace uh::cluster {

template <typename handler>
concept request_handler = requires(handler h, http_request r) {
    { handler::can_handle(r) } -> std::same_as<bool>;
    { h.handle(r) } -> std::same_as<coro<http_response>>;
};

class entrypoint_handler : public protocol_handler {
public:
    explicit entrypoint_handler(
        command_factory&& comm_factory,
        std::unique_ptr<ep::http::request_factory> factory)
        : m_command_factory(comm_factory),
          m_factory(std::move(factory)) {}

    coro<void> on_startup() override {
        m_command_factory.get_limits().storage_size(
            co_await m_command_factory.get_directory().data_size());
    }

    coro<void> handle(boost::asio::ip::tcp::socket s) override {
        for (;;) {

            auto req = co_await m_factory->create(s);
            LOG_DEBUG() << req->peer() << ": read request: " << *req;

            std::optional<http_response> resp;
            bool keep_alive = false;

            try {
                resp = co_await handle_request(*req);
                metric<success>::increase(1);
                keep_alive = req->keep_alive();
            } catch (const command_exception& e) {
                LOG_ERROR() << req->peer() << ": " << e.what();
                resp = make_response(e);
            } catch (const boost::system::system_error& se) {
                if (se.code() != http::error::end_of_stream) {
                    LOG_ERROR() << req->peer() << ": peer closed connection";
                    break;
                }

                LOG_ERROR() << req->peer() << ": " << se.what();
                resp = make_response(command_exception(
                    http::status::bad_request, "BadRequest", "bad request"));
            } catch (const std::exception& e) {
                LOG_ERROR() << req->peer() << ": " << e.what();

                resp = make_response(command_exception());
            }

            if (resp) {
                LOG_DEBUG() << req->peer() << ", sending response: " << *resp;
                co_await write(s, std::move(*resp));
            } else {
                LOG_INFO() << req->peer() << ", no response: disconnecting";
                break;
            }

            if (!keep_alive) {
                break;
            }
        }

        s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        s.close();
    }

    coro<http_response> handle_request(http_request& req) const {

        auto cmd = m_command_factory.create(req);

        co_await cmd->validate(req);

        if (auto expect = req.header("expect");
            expect && *expect == "100-continue") {
            LOG_INFO() << req.peer() << ": sending 100 CONTINUE";
            co_await write(req.socket(),
                           http_response(http::status::continue_));
        }

        co_return co_await cmd->handle(req);
    }

private:
    command_factory m_command_factory;
    std::unique_ptr<ep::http::request_factory> m_factory;
};

} // end namespace uh::cluster

#endif // ENTRYPOINT_HANDLER_H
