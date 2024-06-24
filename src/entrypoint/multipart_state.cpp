#include "multipart_state.h"

#include "common/debug/debug.h"
#include "common/telemetry/log.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

multipart_state::multipart_state(boost::asio::io_context& ioc,
                                 const db::config& cfg)
    : m_db(ioc, connection_factory(ioc, cfg, cfg.multipart),
           cfg.multipart.count) {}

coro<std::string> multipart_state::insert_upload(std::string bucket,
                                                 std::string key) {
    LOG_CORO_CONTEXT();
    auto conn = co_await m_db.get();

    auto row =
        co_await conn->execv("SELECT uh_create_upload($1, $2)", bucket, key);

    auto id = *row->string(0);

    LOG_DEBUG() << "insert upload, id " << id << ", bucket: " << bucket
                << ", key: " << key;

    co_return id;
}

coro<upload_info> multipart_state::details(const std::string& id) {
    LOG_CORO_CONTEXT();
    LOG_DEBUG() << "get upload info, id: " << id;

    auto conn = co_await m_db.get();

    upload_info rv;

    {
        auto row = co_await conn->execv(
            "SELECT bucket, key, erased_since FROM uh_get_upload($1)", id);
        if (!row) {
            throw command_exception(http::status::not_found, "NoSuchUpload",
                                    "upload id not found");
        }

        rv.bucket = *row->string(0);
        rv.key = *row->string(1);
        rv.erased = row->date(2).has_value();
    }

    auto row = co_await conn->execv("SELECT part_id, size, effective_size, "
                                    "etag FROM uh_get_upload_parts($1)",
                                    id);
    for (; row; row = co_await conn->next()) {
        auto id = *row->number(0);
        std::size_t size = *row->number(1);

        rv.parts[id] = upload_info::part{.etag = std::string(*row->string(3)),
                                         .size = size};

        rv.data_size += size;
        rv.effective_size += *row->number(2);
    }

    {
        auto row = co_await conn->execb(
            "SELECT part_id, address FROM uh_get_upload_parts($1)", id);
        for (; row; row = co_await conn->next()) {
            rv.parts[*row->number(0)].addr = to_address(*row->data(1));
        }
    }

    co_return rv;
}

coro<void> multipart_state::append_upload_part_info(const std::string& id,
                                                    uint16_t part,
                                                    const dedupe_response& resp,
                                                    size_t data_size,
                                                    std::string&& md5) {
    LOG_CORO_CONTEXT();

    LOG_DEBUG() << "append upload part info, id: " << id << ", part: " << part;

    auto data = to_buffer(resp.addr);

    auto conn = co_await m_db.get();
    co_await conn->execv("CALL uh_put_multipart($1, $2, $3, $4, $5, $6)", id,
                         part, data_size, resp.effective_size,
                         std::span<char>(data), md5);
}

coro<void> multipart_state::remove_upload(const std::string& id) {
    LOG_CORO_CONTEXT();
    LOG_DEBUG() << "remove upload, id: " << id;

    auto conn = co_await m_db.get();
    co_await conn->execv("CALL uh_delete_upload($1)", id);

    co_await clear_infos(*conn);
}

coro<std::map<std::string, std::string>>
multipart_state::list_multipart_uploads(const std::string& bucket) {
    LOG_CORO_CONTEXT();

    LOG_DEBUG() << "list multipart uploads for bucket " << bucket;

    auto conn = co_await m_db.get();

    std::map<std::string, std::string> rv;

    auto row =
        co_await conn->execv("SELECT id, key FROM uh_get_uploads($1)", bucket);
    for (; row; row = co_await conn->next()) {
        rv[std::string(*row->string(0))] = *row->string(1);
    }

    co_return rv;
}

coro<void> multipart_state::clear_infos(db::connection& conn) {
    LOG_CORO_CONTEXT();
    co_await conn.execv(
        "CALL uh_clean_deleted(MAKE_INTERVAL(0, 0, 0, 0, 0, 0, $1))",
        DEFAULT_TIMEOUT);
}

} // namespace uh::cluster
