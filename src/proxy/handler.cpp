#include "handler.h"

#include <common/utils/downstream_exception.h>
#include <common/utils/random.h>
#include <entrypoint/http/command_exception.h>
#include <proxy/forward.h>

#include <format>

using namespace uh::cluster::ep::http;

namespace uh::cluster::proxy {

coro<void> handler::run() {
    std::string endpoint_host = "localhost";
    std::string endpoint_port = "8080";
    boost::asio::ip::tcp::resolver resolver(m_client.get_executor());
    auto endpoints = co_await resolver.async_resolve(
        endpoint_host, endpoint_port, boost::asio::use_awaitable);
    boost::asio::ip::tcp::socket endpoint(m_client.get_executor());
    LOG_DEBUG() << "Connecting to endpoint " << endpoint_host << ":"
                << endpoint_port;
    co_await boost::asio::async_connect(endpoint, endpoints,
                                        boost::asio::use_awaitable);

    std::optional<std::string> failed_request_id{std::nullopt};

    auto state = co_await boost::asio::this_coro::cancellation_state;
    while (state.cancelled() == boost::asio::cancellation_type::none) {
        // Note: lifetime of response must not exceed lifetime of request.
        std::string id = generate_unique_id();

        std::optional<response> resp;

        try {
            try {
                co_await handle_request(endpoint, id).start_trace();
                metric<success>::increase(1);

            } catch (const boost::system::system_error& e) {
                throw;
            } catch (const downstream_exception& e) {
                if (e.code() == boost::asio::error::operation_aborted) {
                    throw e.original_exception();
                } else if (e.code() == boost::beast::error::timeout) {
                    LOG_WARN() << e.what();
                    resp = make_response(command_exception(error::busy));
                } else {
                    LOG_WARN() << e.what();
                    resp = make_response(
                        command_exception(error::internal_network_error));
                }
            } catch (const command_exception& e) {
                LOG_WARN() << e.what();
                resp = make_response(e);
            } catch (const error_exception& e) {
                LOG_WARN() << e.what();
                resp = make_response(command_exception(*e.error()));
            } catch (const std::exception& e) {
                LOG_WARN() << e.what();
                resp = make_response(command_exception());
            }

            if (resp.has_value()) {
                co_await write(m_client, std::move(resp.value()), id);
            }

        } catch (const boost::system::system_error& e) {
            if (e.code() == boost::asio::error::operation_aborted) {
                failed_request_id = id;
                break;
            } else if (e.code() == boost::beast::http::error::end_of_stream or
                       e.code() == boost::asio::error::eof) {
                LOG_INFO() << m_client.remote_endpoint() << " disconnected";
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
        co_await write(m_client, std::move(resp), *failed_request_id);
    }
}

coro<void> handler::handle_request(boost::asio::ip::tcp::socket& endpoint,
                                   const std::string& id) {
    std::unique_ptr<request> req;
    req = co_await m_factory.m_request_factory.create(m_client);
    LOG_INFO() << req->peer() << ": read request, id=" << id << ": " << *req;

    auto span = co_await boost::asio::this_coro::span;

    span->set_attribute("client-ip", req->peer().address().to_string());
    span->set_attribute("request-target", req->target());
    span->set_attribute("request-user-id", req->authenticated_user().id);
    span->set_attribute("request-user-name", req->authenticated_user().name);
    span->set_attribute("request-bucket", req->bucket());
    span->set_attribute("request-key", req->object_key());

    co_await forward(*req, endpoint);

    // TODO: Implement response backwarding and parsing
}

handler_factory::handler_factory(command_factory&& comm_factory,
                                 request_factory&& req_factory,
                                 std::unique_ptr<ep::policy::module> policy,
                                 std::unique_ptr<ep::cors::module> cors)
    : m_command_factory(std::move(comm_factory)),
      m_request_factory(std::move(req_factory)),
      m_policy(std::move(policy)),
      m_cors(std::move(cors)) {}

} // namespace uh::cluster::proxy
