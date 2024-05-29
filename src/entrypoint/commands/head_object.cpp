#include "head_object.h"

#include "entrypoint/formats.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

head_object::head_object(const reference_collection& coll)
    : m_coll(coll) {}

bool head_object::can_handle(const http_request& req) {
    return req.method() == method::head && !req.bucket().empty() &&
           !req.object_key().empty() && !req.query("attributes");
}

coro<void> head_object::handle(const http_request& req) const {
    metric<entrypoint_head_object_req>::increase(1);

    try {
        auto obj_list = co_await m_coll.directory.list_objects(
            req.bucket(), req.object_key(), std::nullopt);

        if (obj_list.empty()) {
            throw std::runtime_error("not found");
        }

        http::response<http::empty_body> res{http::status::ok, 11};
        res.base().set("Content-Length", std::to_string(obj_list[0].size));
        res.base().set("Last-Modified", imf_fixdate(obj_list[0].last_modified));

        http::response_serializer<http::empty_body> sr(res);
        co_await http::async_write_header(
            req.socket(), sr,
            boost::asio::as_tuple(boost::asio::use_awaitable));
    } catch (const std::exception& e) {
        throw command_exception(http::status::not_found, "NoSuchKey",
                                "object not found");
    }
}

} // namespace uh::cluster
