#include <proxy/cache/cache.h>
#include <proxy/cache/storage_cache.h>

#include <common/utils/strings.h>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;

size_t get_content_length(const http::response<http::empty_body>& resp) {
    auto len_str = resp[http::field::content_length]; // std::string_view 타입
    if (!len_str.empty()) {
        return std::stoull(std::string(len_str));
    }
}

/*
 * Read object and create lazy body, send body to incomming
 */
awaitable<void> read_storage(asio::ip::tcp::socket& incomming,
                             const http::request<http::empty_body>& req,
                             const cache_entry& entry, data_view storage) {
    http::response<http::empty_body> res;
    res.result(http::status::ok);
    res.version(req.version());
    res.set(http::field::content_type, "application/octet-stream");
    res.set(http::field::etag, entry->tag);
    res.keep_alive(req.keep_alive());
    res.content_length(entry->size);

    co_await http::async_write_header(incomming, res);

    auto reader = create_reader(storage, entry->addr, 8192);
    while (true) {
        auto chunk = co_await reader.read();
        if (chunk.empty())
            break;
        co_await asio::async_write(incomming,
                                   asio::buffer(chunk.data(), chunk.size()));
    }
}

/*
 * Get lazy body from outgoing, save body to storage while fowarding them to
 * incomming
 */
awaitable<address> save_object(asio::ip::tcp::socket& outgoing,
                               asio::ip::tcp::socket& incomming,
                               data_view storage) {
    std::vector<address> addresses;
    auto writer = create_writer(storage);
    while (true) {
        co_await http::async_read_some(outgoing, resp_buffer);
        if (resp_buffer.empty())
            break;
        co_await asio::async_write(incomming,
                                   asio::buffer(chunk.data(), chunk.size()));
        auto addr = co_await writer.write(chunk);
        addresses.push_back(addr);
    }
    auto final_addr = concat(addresses);
}

awaitable<void>
send_conditional_request(asio::ip::tcp::socket& outgoing,
                         const http::request<http::empty_body>& req,
                         const std::string& etag) {
    http::request<http::empty_body> cond_req = req;
    cond_req.set(http::field::if_none_match, *etag);
    co_await http::async_write(outgoing, cond_req);
}

coro<size_t>
reading_response_header(asio::ip::tcp::socket& outgoing,
                        const http::request<http::empty_body>& req) {
    beast::http::response_parser<beast::http::empty_body> parser;
    parser.body_limit((std::numeric_limits<std::uint64_t>::max)());

    auto buffer = co_await outgoing.read_until("\r\n\r\n");
    // std::string txt(buffer.data(), buffer.size());
    // boost::replace_all(txt, "\r", "\\r");
    // boost::replace_all(txt, "\n", "\\n");

    beast::error_code ec;
    parser.put(boost::asio::buffer(buffer), ec);

    auto res = parser.release();

    bs = outgoing.buffer_size();
    std::size_t read = 0ull;
    std::size_t len = std::stoul(res.at("Content-Length"));
    if (req.method() == boost::beast::http::verb::head &&
        (res.result_int() / 100 == 2)) {
        len = 0;
    }

    LOG_INFO() << peer << ": sending response " << res.result_int() << " "
               << res.reason() << " -- " << len;
    co_return len;
}

awaitable<void> handler(asio::ip::tcp::socket& incoming,
                        asio::ip::tcp::socket& outgoing, cache& c,
                        data_view storage) {

    auto peer = s.remote_endpoint();

    constexpr size_t storage_size = 1024 * 1024 * 1024; // 1GB
    storage_cache sc(c, storage, storage_size);
    lock_map<object_metadata, object_handle> locks;

    while (true) {
        auto rawreq = co_await raw_request::read(incoming, peer);
        auto req = co_await m_factory->create(incoming, rawreq);

        if (req.method() == http::verb::get) {
            auto key = object_metadata{req.path(), req.query("versionId")};
            auto pobj = sc.get(key);
            if (pobj != nullptr) {
                incoming.set_mode(forward_stream::deleting);
                outgoing.set_mode(forward_stream::deleting);
                write(incoming, get_response(pobj->get_body()), session_id);
                co_await incoming.consume();
                co_await outgoing.consume();
            } else {
                pobj = co_await locks.acquire();
                if (pobj == nullptr) {
                    incoming.set_mode(forward_stream::forwarding);
                    outgoing.set_mode(forward_stream::forwarding);

                    // forwarding request
                    co_await read(req->body());

                    // forwarding response
                    auto length = reading_response_header(outgoing, req);
                    auto body =
                        storage_writer_body{m_dedupe, downstream, length};
                    pobj = object_handle::create(body));
                    co_await outgoing.consume();
                    sc.put(key, pobj);
                } else {
                    incoming.set_mode(forward_stream::deleting);
                    outgoing.set_mode(forward_stream::deleting);
                    co_await incoming.consume();
                    co_await outgoing.consume();
                }
                write(incoming, get_response(pobj->get_body()), session_id);
            }

            // if (pobj != nullptr) {
            //     if (pobj->is_fresh()) {
            //         write(incoming, get_response(pobj->get_body()),
            //         session_id);
            //     } else {
            //         co_await send_conditional_request(outgoing, req,
            //                                           entry.tag);
            //
            //         auto resp = co_await outgoing.read_header();
            //         if (resp.result() == http::status::not_modified) {
            //             pobj->revive();
            //             write(incoming, get_response(pobj->get_body()),
            //                   session_id);
        } else {
            // Not a GET request
        }
    }
}
