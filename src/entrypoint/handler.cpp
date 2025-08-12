#include "handler.h"
#include "http/command_exception.h"
#include <common/utils/downstream_exception.h>
#include <common/utils/random.h>
#include <format>

using namespace uh::cluster::ep::http;

namespace uh::cluster::ep {

handler::handler(command_factory&& comm_factory, request_factory&& factory,
                 std::unique_ptr<ep::policy::module> policy,
                 std::unique_ptr<ep::cors::module> cors)
    : m_command_factory(comm_factory),
      m_factory(std::move(factory)),
      m_policy(std::move(policy)),
      m_cors(std::move(cors)) {}

coro<void> handler::handle(boost::asio::ip::tcp::socket s) {
    std::optional<std::string> failed_request_id{std::nullopt};

    auto state = co_await boost::asio::this_coro::cancellation_state;
    while (state.cancelled() == boost::asio::cancellation_type::none) {
        // Note: lifetime of response must not exceed lifetime of request.
        std::string id = generate_unique_id();

        raw_request rawreq;
        response resp;

        try {
            try {
                rawreq = co_await raw_request::read(s);
                resp = co_await handle_request(s, rawreq, id).start_trace();
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

            co_await write(s, std::move(resp), id);

        } catch (const boost::system::system_error& e) {
            if (e.code() == boost::asio::error::operation_aborted) {
                failed_request_id = id;
                break;
            } else if (e.code() == boost::beast::http::error::end_of_stream or
                       e.code() == boost::asio::error::eof) {
                LOG_INFO() << s.remote_endpoint() << " disconnected";
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
        co_await write(s, std::move(resp), *failed_request_id);
    }
}

coro<response> handler::handle_request(boost::asio::ip::tcp::socket& s,
                                       raw_request& rawreq,
                                       const std::string& id) {
    std::unique_ptr<request> req;
    req = co_await m_factory.create(s, rawreq);
    LOG_INFO() << req->peer() << ": read request, id=" << id << ": " << *req;

    auto span = co_await boost::asio::this_coro::span;

    span->set_attribute("client-ip", req->peer().address().to_string());
    span->set_attribute("request-target", req->target());
    span->set_attribute("request-user-id", req->authenticated_user().id);
    span->set_attribute("request-user-name", req->authenticated_user().name);
    span->set_attribute("request-bucket", req->bucket());
    span->set_attribute("request-key", req->object_key());

    auto cors = co_await m_cors->check(*req);
    if (cors.response) {
        co_return std::move(*cors.response);
    }

    auto cmd = co_await m_command_factory.create(*req);

    span->set_name(cmd->action_id());
    span->set_attribute("request-id", id);

    LOG_DEBUG() << req->peer() << ": validating " << cmd->action_id();

    LOG_DEBUG() << req->peer() << ": checking policies";
    if (!req->authenticated_user().super_user &&
        co_await m_policy->check(*req, *cmd) == ep::policy::effect::deny) {
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
        co_await write(s, response(status::continue_), id);
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

    co_return response;
}

} // namespace uh::cluster::ep
