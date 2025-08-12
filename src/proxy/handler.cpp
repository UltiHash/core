#include "handler.h"
#include <entrypoint/http/command_exception.h>
#include <common/utils/downstream_exception.h>
#include <common/utils/random.h>
#include "forward_command.h"
#include <format>

using namespace uh::cluster::proxy::http;

namespace uh::cluster::proxy {

handler::handler(std::unique_ptr<request_factory> factory,
                 std::function<std::unique_ptr<boost::beast::tcp_stream>()> sf,
                 std::size_t buffer_size)
    : m_factory(std::move(factory)),
      m_sf(std::move(sf)),
      m_buffer_size(buffer_size) {}

coro<void> handler::handle(boost::asio::ip::tcp::socket s) {
    auto downstream = m_sf();
    for (;;) {

        /*
         * Note: lifetime of response must not exceed lifetime of request.
         */
        std::string id = generate_unique_id();

        raw_request rawreq;
        response resp;

        try {
            try {
                rawreq = co_await raw_request::read(s);
                resp = co_await handle_request(s, rawreq, id, *downstream).start_trace();
                metric<success>::increase(1);

            } catch (const boost::system::system_error& e) {
                throw;
            } catch (const downstream_exception& e) {
                if (e.code() == boost::asio::error::operation_aborted or
                    e.code() == boost::beast::error::timeout) {
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
                LOG_ERROR() << s.remote_endpoint() << ": " << e.what();
                resp = make_response(command_exception());
            }

            co_await write(s, std::move(resp), id);

        } catch (const boost::system::system_error& e) {
            if (e.code() == boost::beast::http::error::end_of_stream or
                e.code() == boost::asio::error::eof) {
                LOG_INFO() << s.remote_endpoint() << " disconnected";
                break;
            }
            throw;
        }
    }

    s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    s.close();
}

coro<response> handler::handle_request(boost::asio::ip::tcp::socket& s,
                                       raw_request& rawreq,
                                       const std::string& id,
                                       boost::beast::tcp_stream& ds) {
    std::unique_ptr<request> req;
    req = co_await m_factory->create(s, rawreq);
    LOG_INFO() << req->peer() << ": read request, id=" << id << ": " << *req;

    auto span = co_await boost::asio::this_coro::span;

    span->set_attribute("client-ip", req->peer().address().to_string());
    span->set_attribute("request-target", req->target());
    span->set_attribute("request-user-id", req->authenticated_user().id);
    span->set_attribute("request-user-name", req->authenticated_user().name);
    span->set_attribute("request-bucket", req->bucket());
    span->set_attribute("request-key", req->object_key());

    auto cmd = std::make_unique<forward_command>(ds, m_buffer_size);

    span->set_name(cmd->action_id());
    span->set_attribute("request-id", id);

    LOG_DEBUG() << req->peer() << ": validating " << cmd->action_id();
    co_await cmd->validate(*req);

    if (auto expect = req->header("expect");
        expect && *expect == "100-continue") {
        LOG_INFO() << req->peer() << ": sending 100 CONTINUE";
        co_await write(s, response(status::continue_), id);
    }

    LOG_DEBUG() << req->peer() << ": executing " << cmd->action_id();
    auto response = co_await cmd->handle(*req);
    span->set_attribute("response-code",
                        static_cast<unsigned>(response.base().result()));

    co_return response;
}

} // namespace uh::cluster::proxy
