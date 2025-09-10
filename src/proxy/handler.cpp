#include "handler.h"

#include "forward_stream.h"

#include <common/telemetry/metrics.h>
#include <common/utils/random.h>
#include <entrypoint/commands/s3/get_object.h>
#include <entrypoint/http/command_exception.h>
#include <entrypoint/http/response.h>
#include <proxy/cache/asio.h>

using namespace uh::cluster::ep::http;

namespace uh::cluster::proxy {

handler::handler(
    std::unique_ptr<request_factory> factory,
    std::function<std::unique_ptr<boost::asio::ip::tcp::socket>()> sf,
    storage::data_view& dv, cache::disk::manager& mgr, std::size_t buffer_size)
    : m_factory(std::move(factory)),
      m_sf(std::move(sf)),
      m_dv(dv),
      m_mgr(mgr),
      m_buffer_size(buffer_size) {}

coro<void> handler::handle(boost::asio::ip::tcp::socket s) {
    auto ds = m_sf();
    auto peer = s.remote_endpoint();

    forward_stream incoming(s, *ds);
    forward_stream outgoing(*ds, s);
    for (;;) {

        /*
         * Note: lifetime of response must not exceed lifetime of request.
         */
        std::string id = generate_unique_id();

        raw_request rawreq;
        std::optional<response> resp;

        try {
            rawreq = co_await raw_request::read(incoming, peer);

            auto& r = rawreq.headers;
            LOG_INFO() << peer << ": incoming request: " << r.method_string()
                       << " " << r.target();

            incoming.set_mode(forward_stream::forwarding);
            outgoing.set_mode(forward_stream::forwarding);
            std::unique_ptr<request> req =
                co_await m_factory->create(incoming, rawreq);

            if (get_object::can_handle(*req)) {
                auto wbody =
                    m_mgr.get(cache::disk::object_metadata{req->object_key()});
                if (wbody) {
                    LOG_INFO() << peer << ": handling from cache";
                    incoming.set_mode(forward_stream::deleting);
                    outgoing.set_mode(forward_stream::deleting);

                    // forwarding request
                    auto& b = req->body();
                    auto bs = b.buffer_size();

                    while (!(co_await b.read(bs)).empty()) {
                        co_await b.consume();
                    }

                    co_await b.consume();

                    LOG_INFO() << peer << ": done reading complete request";

                    co_await cache::async_write(incoming, *wbody);

                    LOG_INFO() << peer << ": cache result served";
                    continue;
                }
            }

            LOG_INFO() << peer << ": handling from downstream";
            co_await incoming.consume();

            if (auto expect = req->header("expect");
                expect && *expect == "100-continue") {
                LOG_INFO() << req->peer() << ": forwarding 100 CONTINUE";
                // TODO timeout
                co_await outgoing.read_until("\r\n\r\n");
                co_await outgoing.consume();
            }

            // forwarding request
            auto& b = req->body();
            auto bs = b.buffer_size();

            while (!(co_await b.read(bs)).empty()) {
                co_await b.consume();
            }

            co_await b.consume();

            // forwarding response
            beast::http::response_parser<beast::http::empty_body> parser;
            parser.body_limit((std::numeric_limits<std::uint64_t>::max)());

            auto buffer = co_await outgoing.read_until("\r\n\r\n");

            beast::error_code ec;
            parser.put(boost::asio::buffer(buffer), ec);

            auto res = parser.release();

            bs = outgoing.buffer_size();
            std::size_t len = std::stoul(res.at("Content-Length"));
            if (r.method() == boost::beast::http::verb::head &&
                (res.result_int() / 100 == 2)) {
                len = 0;
            }

            LOG_INFO() << peer << ": sending response " << res.result_int()
                       << " " << res.reason() << " -- " << len;

            if (get_object::can_handle(*req)) {
                LOG_INFO() << peer << ": add " << buffer.size()
                           << " response header";
                cache::disk::reader_body rbody(m_dv);
                co_await rbody.put(buffer);

                co_await cache::async_read(outgoing, rbody, len);

                co_await m_mgr.put(
                    cache::disk::object_metadata{req->object_key()}, rbody);
            } else {
                std::size_t read = 0ull;
                while (read < len) {
                    co_await outgoing.consume();

                    auto r = co_await outgoing.read(len - read);
                    // r: rbody
                    read += r.size();
                }

                co_await outgoing.consume();
            }

            metric<success>::increase(1);

        } catch (const boost::system::system_error& e) {
            throw;
        } catch (const command_exception& e) {
            resp = make_response(e);
        } catch (const error_exception& e) {
            resp = make_response(command_exception(*e.error()));
        } catch (const std::exception& e) {
            LOG_ERROR() << s.remote_endpoint() << ": " << e.what();
            resp = make_response(command_exception());
        }

        if (resp) {
            co_await write(incoming, std::move(*resp), id);
        }
    }

    s.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    s.close();
}

bool handler::intercept(ep::http::raw_request& r) const { return false; }

coro<void> handler::handle(ep::http::stream& s, ep::http::raw_request& r) {
    co_return;
}

} // namespace uh::cluster::proxy
