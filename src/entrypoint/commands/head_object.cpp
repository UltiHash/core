#include "head_object.h"

#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

head_object::head_object(const reference_collection& coll)
    : m_coll(coll) {}

bool head_object::can_handle(const http_request& req) {
    const auto& uri = req.get_uri();
    return req.get_method() == method::head && !uri.get_bucket_id().empty() &&
           !uri.get_object_key().empty() &&
           !uri.query_string_exists("attributes");
}

coro<void> head_object::handle(const http_request& req) const {
    metric<entrypoint_head_object_req>::increase(1);

    auto client = m_coll.directory_services.get();
    auto m = co_await client->acquire_messenger();

    try {
        directory_message dir_req;
        dir_req.bucket_id = req.get_uri().get_bucket_id();
        dir_req.object_key_prefix =
            std::make_unique<std::string>(req.get_uri().get_object_key());

        co_await m->send_directory_message(DIRECTORY_OBJECT_LIST_REQ, dir_req);
        auto hdr = co_await m->recv_header();
        auto lst = co_await m->recv_directory_list_objects_message(hdr);

        if (lst.objects.empty()) {
            throw std::runtime_error("not found");
        }

        http::response<http::empty_body> res{http::status::ok, 11};
        res.base().set("Content-Length", std::to_string(lst.objects[0].size));
        res.base().set("Last-Modified", "Sun, 1 Jan 2006 12:00:00 GMT");

        http::response_serializer<http::empty_body> sr(res);
        co_await http::async_write_header(
            req.socket(), sr,
            boost::asio::as_tuple(boost::asio::use_awaitable));
    } catch (const std::exception& e) {
        throw command_exception(http::status::not_found,
                                command_error::object_not_found);
    }
}

} // namespace uh::cluster
