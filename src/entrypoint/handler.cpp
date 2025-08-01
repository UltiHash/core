#include "handler.h"
#include "http/command_exception.h"
#include <common/utils/downstream_exception.h>
#include <common/utils/random.h>
#include <format>

using namespace uh::cluster::ep::http;

namespace uh::cluster::ep {

coro<void> handler::run() {
    std::optional<std::string> failed_request_id{std::nullopt};

    auto state = co_await boost::asio::this_coro::cancellation_state;
    while (state.cancelled() == boost::asio::cancellation_type::none) {
        // Note: lifetime of response must not exceed lifetime of request.
        std::string id = generate_unique_id();

        raw_request rawreq;
        std::optional<response> resp;

        try {
            try {
                rawreq = co_await raw_request::read(m_socket);
                co_await handle_request(rawreq, id).start_trace();
                metric<success>::increase(1);

            } catch (const boost::system::system_error& e) {
                throw;
            } catch (const downstream_exception& e) {
                if (e.code() == boost::asio::error::operation_aborted) {
                    throw e.original_exception();
                } else if (e.code() == boost::beast::error::timeout) {
                    resp = make_response(command_exception(error::busy));
                } else {
                    resp = make_response(
                        command_exception(error::internal_network_error));
                }
            } catch (const command_exception& e) {
                resp = make_response(e);
            } catch (const error_exception& e) {
                resp = make_response(command_exception(*e.error()));
            } catch (const std::exception& e) {
                resp = make_response(command_exception());
            }

            if (resp.has_value()) {
                co_await write(m_socket, std::move(resp.value()), id);
            }

        } catch (const boost::system::system_error& e) {
            if (e.code() == boost::asio::error::operation_aborted) {
                failed_request_id = id;
                break;
            } else if (e.code() == boost::beast::http::error::end_of_stream or
                       e.code() == boost::asio::error::eof) {
                LOG_INFO() << m_socket.remote_endpoint() << " disconnected";
                break;
            }
            throw;
        }
    }

    if (failed_request_id) {
        co_await boost::asio::this_coro::reset_cancellation_state(
            boost::asio::disable_cancellation());

        auto resp =
            make_response(command_exception(error::service_unavailable));
        co_await write(m_socket, std::move(resp), *failed_request_id);
    }
}

coro<void> handler::proxy_raw_request(raw_request& rawreq) {
    constexpr std::size_t buffer_size = 64 * 1024; // 64KB

    std::string endpoint_host = "localhost";
    std::string endpoint_port = "8080";
    boost::asio::ip::tcp::resolver resolver(m_socket.get_executor());
    auto endpoints = co_await resolver.async_resolve(
        endpoint_host, endpoint_port, boost::asio::use_awaitable);
    boost::asio::ip::tcp::socket endpoint_socket(m_socket.get_executor());
    co_await boost::asio::async_connect(endpoint_socket, endpoints,
                                        boost::asio::use_awaitable);

    co_await boost::asio::async_write(
        endpoint_socket,
        boost::asio::buffer(rawreq.buffer.data(), rawreq.buffer.size()),
        boost::asio::use_awaitable);

    std::vector<char> buffer(buffer_size);
    for (;;) {
        std::size_t n = co_await m_socket.async_read_some(
            boost::asio::buffer(buffer), boost::asio::use_awaitable);
        if (n == 0)
            break;
        co_await boost::asio::async_write(endpoint_socket,
                                          boost::asio::buffer(buffer.data(), n),
                                          boost::asio::use_awaitable);
    }

    for (;;) {
        std::size_t n = co_await endpoint_socket.async_read_some(
            boost::asio::buffer(buffer), boost::asio::use_awaitable);
        if (n == 0)
            break;
        co_await boost::asio::async_write(m_socket,
                                          boost::asio::buffer(buffer.data(), n),
                                          boost::asio::use_awaitable);
    }
}

coro<void> handler::handle_request(raw_request& rawreq, const std::string& id) {
    co_await proxy_raw_request(rawreq);
    co_return;

    std::unique_ptr<request> req;
    req = co_await m_factory.m_request_factory.create(m_socket, rawreq);
    LOG_INFO() << req->peer() << ": read request, id=" << id << ": " << *req;

    auto span = co_await boost::asio::this_coro::span;

    span->set_attribute("client-ip", req->peer().address().to_string());
    span->set_attribute("request-target", req->target());
    span->set_attribute("request-user-id", req->authenticated_user().id);
    span->set_attribute("request-user-name", req->authenticated_user().name);
    span->set_attribute("request-bucket", req->bucket());
    span->set_attribute("request-key", req->object_key());

    auto cors = co_await m_factory.m_cors->check(*req);
    if (cors.response) {
        co_await write(m_socket, std::move(*cors.response), id);
        co_return;
    }

    auto cmd = co_await m_factory.m_command_factory.create(*req);

    span->set_name(cmd->action_id());
    span->set_attribute("request-id", id);

    LOG_DEBUG() << req->peer() << ": validating " << cmd->action_id();

    LOG_DEBUG() << req->peer() << ": checking policies";
    if (!req->authenticated_user().super_user &&
        co_await m_factory.m_policy->check(*req, *cmd) ==
            ep::policy::effect::deny) {
        LOG_INFO() << req->peer() << ": command execution denied by policy";
        throw command_exception(
            status::forbidden, "AccessDenied",
            std::format("You do not have {} permissions to the requested "
                        "S3 Prefix{}.",
                        cmd->action_id(), req->path()));
    }

    co_await cmd->validate(*req);

    if (auto expect = req->header("expect");
        expect && *expect == "100-continue") {
        LOG_INFO() << req->peer() << ": sending 100 CONTINUE";
        co_await write(m_socket, response(status::continue_), id);
    }

    LOG_DEBUG() << req->peer() << ": executing " << cmd->action_id();
    auto response = co_await cmd->handle(*req);
    if (cors.headers) {
        for (auto& hdr : *cors.headers) {
            response.set(hdr.first, std::move(hdr.second));
        }
    }

    span->set_attribute("response-code",
                        static_cast<unsigned>(response.base().result()));

    co_await write(m_socket, std::move(response), id);
}

handler_factory::handler_factory(command_factory&& comm_factory,
                                 request_factory&& req_factory,
                                 std::unique_ptr<ep::policy::module> policy,
                                 std::unique_ptr<ep::cors::module> cors)
    : m_command_factory(std::move(comm_factory)),
      m_request_factory(std::move(req_factory)),
      m_policy(std::move(policy)),
      m_cors(std::move(cors)) {}

} // namespace uh::cluster::ep
