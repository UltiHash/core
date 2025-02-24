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
        std::unique_ptr<request> req;
        std::string id = generate_unique_id();
        response resp;
        bool keep_alive = false;

        try {
            req = co_await m_factory.create(s);
            LOG_INFO() << req->peer() << ": read request, id=" << id << ": "
                       << *req;

            req->context().set_attribute("request-id", id);

            resp = co_await handle_request(s, *req, id);
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
            }

            LOG_ERROR() << s.remote_endpoint() << ": " << se.what();
            resp = make_response(
                command_exception(status::internal_server_error,
                                  "InternalError", "Internal server error."));
        } catch (const std::exception& e) {
            LOG_ERROR() << s.remote_endpoint() << ": " << e.what();

            resp = make_response(command_exception());
        }

        if (req) {
            req->context().set_attribute(
                "response-code", static_cast<unsigned>(resp.base().result()));
        }

        co_await write(s, std::move(resp), id);
        if (!keep_alive) {
            break;
        }
    }

    s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    s.close();
}

coro<response> handler::handle_request(boost::asio::ip::tcp::socket& s,
                                       request& req, const std::string& id) {

    auto cors = co_await m_cors->check(req);
    if (cors.response) {
        co_return std::move(*cors.response);
    }

    auto cmd = co_await m_command_factory.create(req);
    LOG_DEBUG() << req.peer() << ": validating " << cmd->action_id();

    req.context().set_name(cmd->action_id());

    LOG_DEBUG() << req.peer() << ": checking policies";
    if (!req.authenticated_user().super_user &&
        co_await m_policy->check(req, *cmd) == ep::policy::effect::deny) {
        LOG_INFO() << req.peer() << ": command execution denied by policy";
        throw command_exception(
            status::forbidden, "AccessDenied",
            std::format(
                "You do not have {} permissions to the requested S3 Prefix{}.",
                cmd->action_id(), req.path()));
    }

    co_await cmd->validate(req);

    if (auto expect = req.header("expect");
        expect && *expect == "100-continue") {
        LOG_INFO() << req.peer() << ": sending 100 CONTINUE";
        co_await write(s, response(status::continue_), id);
    }

    LOG_DEBUG() << req.peer() << ": executing " << cmd->action_id();
    auto response = co_await cmd->handle(req);
    if (cors.headers) {
        for (auto& hdr : *cors.headers) {
            response.set(hdr.first, std::move(hdr.second));
        }
    }

    co_return response;
}

} // namespace uh::cluster::ep
