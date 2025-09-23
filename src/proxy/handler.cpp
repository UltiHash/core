#include "handler.h"

#include "forward_stream.h"

#include <proxy/asio.h>
#include <proxy/http.h>

#include <proxy/cache/disk/body.h>
#include <proxy/cache/disk/http.h>

#include <common/telemetry/metrics.h>
#include <common/utils/random.h>
#include <entrypoint/commands/s3/get_object.h>
#include <entrypoint/http/command_exception.h>
#include <entrypoint/http/response.h>

using namespace boost::beast;
using namespace boost::beast::http;

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

    constexpr std::size_t buffer_size_to_load = 16_MiB;

    constexpr std::size_t buffer_size_to_relay_and_store = 32_MiB;
    constexpr std::size_t buffer_size_to_relay = 4_KiB;

    flat_buffer o_buffer(
        std::max(buffer_size_to_relay, buffer_size_to_relay_and_store));

    for (;;) {
        /*
         * Note: lifetime of response must not exceed lifetime of request.
         */
        std::string id = generate_unique_id();

        ep::http::raw_request rawreq;
        std::optional<ep::http::response> resp;

        try {
            rawreq = co_await ep::http::raw_request::read(incoming, peer);

            auto& r = rawreq.headers;
            LOG_INFO() << peer << ": incoming request: " << r.method_string()
                       << " " << r.target();

            incoming.set_mode(forward_stream::forwarding);
            std::unique_ptr<ep::http::request> req =
                co_await m_factory->create(incoming, rawreq);

            if (get_object::can_handle(*req)) {
                auto reader =
                    m_mgr.get(cache::disk::object_metadata{req->object_key()});
                if (reader) {
                    LOG_INFO() << peer << ": handling from cache";
                    incoming.set_mode(forward_stream::deleting);

                    auto& b = req->body();
                    auto bs = b.buffer_size();

                    while (!(co_await b.read(bs)).empty()) {
                        co_await b.consume();
                    }

                    co_await b.consume();

                    LOG_INFO() << peer << ": done reading complete request";

                    response_parser<empty_body> parser;
                    response_serializer<empty_body, fields> serializer{
                        parser.get()};

                    co_await async_read_header(reader, parser);

                    const char* via_value = PROJECT_NAME " " PROJECT_VERSION;
                    parser.get().set(field::via, via_value);

                    co_await async_write_header(s, serializer);

                    co_await async_write<buffer_size_to_load>(s, *reader);

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
                response_parser<empty_body> p;
                response_serializer<empty_body, fields> sr{p.get()};
                co_await async_read_header(outgoing, o_buffer, p);
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
            response_parser<empty_body> p;
            p.body_limit(std::numeric_limits<std::uint64_t>::max());
            response_serializer<empty_body, fields> sr{p.get()};

            LOG_INFO() << peer << ": reading header from downstream";
            co_await async_read_header(outgoing, o_buffer, p);
            // co_await async_write_header(s, sr);

            if (r.method() == verb::head) {
                LOG_INFO() << peer << ": HEAD request, skipping body relay";
                sr.split(true);
                co_await async_write_header(s, sr);

            } else if (get_object::can_handle(*req)) {
                auto writer = cache::disk::writer{m_dv};
                co_await async_write_store_header(s, sr, writer);
                auto body_size = get_content_length(p.get());
                if (!body_size.has_value()) {
                    throw std::runtime_error("no content length");
                }
                co_await async_relay_store_body<buffer_size_to_relay_and_store>(
                    outgoing, s, o_buffer, writer, *body_size);
                co_await m_mgr.put(
                    cache::disk::object_metadata{req->object_key()}, writer);

            } else {
                co_await async_write_header(s, sr);
                auto body_size = get_content_length(p.get());
                if (!body_size.has_value()) {
                    throw std::runtime_error("no content length");
                }
                co_await async_relay_buffer<buffer_size_to_relay>(
                    outgoing, s, o_buffer, *body_size);
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
