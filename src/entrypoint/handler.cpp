#include "handler.h"
#include "http/command_exception.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster::ep {

handler::handler(command_factory&& comm_factory,
                 std::unique_ptr<request_factory> factory,
                 std::unique_ptr<ep::policy::module> policy)
    : m_command_factory(comm_factory),
      m_factory(std::move(factory)),
      m_policy(std::move(policy)) {}

coro<void> handler::on_startup() {
    m_command_factory.get_limits().storage_size(
        co_await m_command_factory.get_directory().data_size());
}

coro<void> handler::handle(boost::asio::ip::tcp::socket s) {
    for (;;) {

        /*
         * Note: livetime of response must not exceed livetime of request.
         */
        std::unique_ptr<request> req;
        response resp;
        bool keep_alive = false;

        try {
            req = co_await m_factory->create(s);
            LOG_DEBUG() << req->peer() << ": read request: " << *req;

            resp = co_await handle_request(s, *req);
            metric<success>::increase(1);
            keep_alive = true;
        } catch (const command_exception& e) {
            LOG_INFO() << s.remote_endpoint() << ": " << e.what();
            resp = make_response(e);
            keep_alive = true;
        } catch (const boost::system::system_error& se) {
            if (se.code() == boost::beast::http::error::end_of_stream) {
                LOG_ERROR()
                    << s.remote_endpoint() << ": peer closed connection";
                break;
            }

            LOG_ERROR() << s.remote_endpoint() << ": " << se.what();
            resp = make_response(command_exception(
                status::bad_request, "BadRequest", "bad request"));
        } catch (const std::exception& e) {
            LOG_ERROR() << s.remote_endpoint() << ": " << e.what();

            resp = make_response(command_exception());
        }

        co_await write(s, std::move(resp));
        if (!keep_alive) {
            break;
        }
    }

    s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    s.close();
}

coro<response> handler::handle_request(boost::asio::ip::tcp::socket& s,
                                       request& req) const {

    auto cmd = m_command_factory.create(req);

    co_await cmd->validate(req);

    if (m_policy->check(req, *cmd) == ep::policy::effect::deny) {
        LOG_INFO() << req.peer() << ": command execution denied by policy";
        throw command_exception(status::forbidden, "AccessDenied",
                                "Access Denied");
    }

    if (auto expect = req.header("expect");
        expect && *expect == "100-continue") {
        LOG_INFO() << req.peer() << ": sending 100 CONTINUE";
        co_await write(s, response(status::continue_));
    }

    co_return co_await cmd->handle(req);
}

} // namespace uh::cluster::ep
