#include "handler.h"
#include "http/command_exception.h"
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
    for (;;) {

        /*
         * Note: lifetime of response must not exceed lifetime of request.
         */
        std::string id = generate_unique_id();

        raw_request rawreq;
        response resp;
        bool keep_alive = false;

        try {
            rawreq = co_await raw_request::read(s);
            resp = co_await handle_request(s, rawreq, id).start_trace();
            metric<success>::increase(1);
            keep_alive = true;

        } catch (const command_exception& e) {
            LOG_INFO() << s.remote_endpoint() << ": " << e.what();
            resp = make_response(e);
            keep_alive = true;

        } catch (const boost::system::system_error& se) {
            if (se.code() == boost::beast::http::error::end_of_stream) {
                LOG_INFO() << s.remote_endpoint() << ": peer closed connection";
                break;
            } else {
                LOG_ERROR() << s.remote_endpoint() << ": " << se.what();
                resp = make_response(command_exception(
                    status::internal_server_error, "InternalError",
                    "Internal server error."));
            }
        } catch (const std::exception& e) {
            LOG_ERROR() << s.remote_endpoint() << ": " << e.what();
            resp = make_response(command_exception());
        }

        try {
            co_await write(s, std::move(resp), id);
        } catch (const boost::system::system_error& se) {
            if (se.code() == boost::beast::http::error::end_of_stream) {
                LOG_INFO() << s.remote_endpoint() << ": peer closed connection";
                break;
            }
            throw;
        }

        if (!keep_alive) {
            break;
        }
    }

    s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    s.close();
}

coro<response> handler::handle_request(boost::asio::ip::tcp::socket& s,
                                       raw_request& rawreq,
                                       const std::string& id) {

    auto req = co_await m_factory.create(s, rawreq);
    LOG_INFO() << ": read request, id=" << id << ": " << *req;

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
