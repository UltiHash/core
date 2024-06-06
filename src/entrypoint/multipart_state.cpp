#include "multipart_state.h"

#include "common/telemetry/log.h"
#include "common/utils/random.h"
#include "entrypoint/http/command_exception.h"

namespace uh::cluster {

multipart_state::multipart_state(boost::asio::io_context& ioc,
                                 const db::config& cfg)
    : m_db(ioc, connection_factory(cfg, cfg.multipart), cfg.multipart.count) {}

coro<std::string> multipart_state::insert_upload(std::string bucket,
                                                 std::string key) {
    auto conn = co_await m_db.get();

    clear_infos(*conn);

    auto res = conn->execv("SELECT uh_create_upload($1, $2)", bucket, key);
    auto id = *res.string(0, 0);

    LOG_DEBUG() << "insert upload, id " << id << ", bucket: " << bucket
                << ", key: " << key;

    co_return id;
}

coro<std::shared_ptr<upload_info>>
multipart_state::get_upload_info(const std::string& id) {
    LOG_DEBUG() << "get upload info, id: " << id;

    auto conn = co_await m_db.get();

    auto rv = std::make_shared<upload_info>();
    {
        auto res = conn->execv(
            "SELECT bucket, key, erased_since FROM uh_get_upload($1)", id);
        if (res.rows() != 1) {
            throw command_exception(http::status::not_found, "NoSuchUpload",
                                    "upload id not found");
        }

        rv->bucket = *res.string(0, 0);
        rv->key = *res.string(0, 1);
        rv->erased = res.date(0, 2).has_value();
    }

    auto res = conn->execv("SELECT part_id, size, effective_size, etag FROM "
                           "uh_get_upload_parts($1)",
                           id);
    for (auto row = 0ull; row < res.rows(); ++row) {
        auto id = *res.number(row, 0);
        auto size = *res.number(row, 1);
        auto eff_size = *res.number(row, 2);

        rv->data_size += size;
        rv->effective_size += eff_size;

        rv->etags[id] = *res.string(row, 3);
        rv->part_sizes[id] = size;
    }

    {
        auto res = conn->execb(
            "SELECT part_id, address FROM uh_get_upload_parts($1)", id);
        for (auto row = 0ull; row < res.rows(); ++row) {
            auto id = *res.number(row, 0);
            rv->addresses.emplace(id, to_address(*res.data(row, 1)));
        }
    }

    co_return rv;
}

coro<void> multipart_state::append_upload_part_info(const std::string& id,
                                                    uint16_t part,
                                                    const dedupe_response& resp,
                                                    size_t data_size,
                                                    std::string&& md5) {

    LOG_DEBUG() << "append upload part info, id: " << id << ", part: " << part;

    auto data = to_buffer(resp.addr);

    auto conn = co_await m_db.get();
    conn->execv("CALL uh_put_multipart($1, $2, $3, $4, $5, $6)", id, part,
                data_size, resp.effective_size, std::span<char>(data), md5);
}

coro<void> multipart_state::remove_upload(const std::string& id) {
    LOG_DEBUG() << "remove upload, id: " << id;

    auto conn = co_await m_db.get();
    conn->execv("CALL uh_delete_upload($1)", id);

    clear_infos(*conn);
}

coro<std::map<std::string, std::string>>
multipart_state::list_multipart_uploads(const std::string& bucket) {

    LOG_DEBUG() << "list multipart uploads for bucket " << bucket;

    auto conn = co_await m_db.get();
    auto res = conn->execv("SELECT id, key FROM uh_get_uploads($1)", bucket);

    std::map<std::string, std::string> rv;
    for (auto row = 0ull; row < res.rows(); ++row) {
        rv[std::string(*res.string(row, 0))] = *res.string(row, 1);
    }

    co_return rv;
}

void multipart_state::clear_infos(db::connection& conn) {
    conn.execv("CALL uh_clean_deleted(MAKE_INTERVAL($1))", DEFAULT_TIMEOUT);
}

} // namespace uh::cluster
