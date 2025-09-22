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
    auto& outgoing{*ds};
    boost::beast::flat_buffer o_buffer;
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
            std::unique_ptr<request> req =
                co_await m_factory->create(incoming, rawreq);

            if (get_object::can_handle(*req)) {
                auto reader =
                    m_mgr.get(cache::disk::object_metadata{req->object_key()});
                if (reader) {
                    LOG_INFO() << peer << ": handling from cache";
                    incoming.set_mode(forward_stream::deleting);

                    // TODO: forwarding request
                    auto& b = req->body();
                    auto bs = b.buffer_size();

                    while (!(co_await b.read(bs)).empty()) {
                        co_await b.consume();
                    }

                    co_await b.consume();

                    LOG_INFO() << peer << ": done reading complete request";
                    auto header_size =
                        std::vector<char>(reader->get_header_size());
                    auto header = co_await reader->get(header_size);

                    boost::beast::http::response_parser<
                        boost::beast::http::empty_body>
                        parser;
                    boost::beast::http::response_serializer<
                        boost::beast::http::empty_body,
                        boost::beast::http::fields>
                        serializer{parser.get()};

                    parser.body_limit(
                        std::numeric_limits<std::uint64_t>::max());
                    boost::system::error_code ec;
                    parser.put(boost::asio::const_buffer(header), ec);
                    if (ec) {
                        throw boost::system::system_error(ec);
                    }

                    co_await async_write_header(s, serializer);

                    co_await cache::async_write<16_MiB>(incoming, *reader);

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
                boost::beast::http::response_parser<
                    boost::beast::http::empty_body>
                    p;
                boost::beast::http::response_serializer<
                    boost::beast::http::empty_body, boost::beast::http::fields>
                    sr{p.get()};
                co_await boost::beast::http::async_read_header(outgoing,
                                                               o_buffer, p);
                co_await async_write_header(s, sr);
            }

            // forwarding request body
            auto& b = req->body();
            auto bs = b.buffer_size();

            while (!(co_await b.read(bs)).empty()) {
                co_await b.consume();
            }

            co_await b.consume();

            // forwarding response
            // TODO: alias default parser and serializer for relaying
            boost::beast::http::response_parser<boost::beast::http::empty_body>
                p;
            p.body_limit(std::numeric_limits<std::uint64_t>::max());
            boost::beast::http::response_serializer<
                boost::beast::http::empty_body, boost::beast::http::fields>
                sr{p.get()};

            LOG_INFO() << peer << ": reading header from downstream";
            co_await boost::beast::http::async_read_header(outgoing, o_buffer,
                                                           p);
            // co_await async_write_header(s, sr);

            if (r.method() == boost::beast::http::verb::head) {
                LOG_INFO() << peer << ": HEAD request, skipping body relay";
                sr.split(true);
                co_await boost::beast::http::async_write_header(s, sr);

            } else if (get_object::can_handle(*req)) {
                cache::disk::writer writer(m_dv);
                co_await cache::async_write_store_header(s, sr, writer);
                auto body_size = cache::get_content_length(p.get());
                if (!body_size.has_value()) {
                    throw std::runtime_error("no content length");
                }
                co_await cache::async_relay_store_body<32_MiB>(
                    outgoing, s, o_buffer, writer, *body_size);
                co_await m_mgr.put(
                    cache::disk::object_metadata{req->object_key()}, writer);

            } else {
                co_await boost::beast::http::async_write_header(s, sr);
                auto body_size = cache::get_content_length(p.get());
                if (!body_size.has_value()) {
                    throw std::runtime_error("no content length");
                }
                co_await cache::async_relay_body<4_KiB>(outgoing, s, o_buffer,
                                                        *body_size);
            }
            LOG_INFO() << peer << ": done";

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
