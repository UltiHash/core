#include "handler.h"

#include <common/utils/downstream_exception.h>
#include <common/utils/random.h>
#include <entrypoint/http/command_exception.h>
#include <format>

using namespace uh::cluster::ep::http;

namespace uh::cluster::proxy {

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
                rawreq = co_await raw_request::read(m_client);
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

coro<void> handler::handle_request(raw_request& rawreq, const std::string& id) {
    std::unique_ptr<request> req;
    req = co_await m_factory.m_request_factory.create(m_client, rawreq);
    LOG_INFO() << req->peer() << ": read request, id=" << id << ": " << *req;

    auto span = co_await boost::asio::this_coro::span;

    span->set_attribute("client-ip", req->peer().address().to_string());
    span->set_attribute("request-target", req->target());
    span->set_attribute("request-user-id", req->authenticated_user().id);
    span->set_attribute("request-user-name", req->authenticated_user().name);
    span->set_attribute("request-bucket", req->bucket());
    span->set_attribute("request-key", req->object_key());
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
