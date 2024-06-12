#include "directory.h"

#include "common/utils/debug.h"
#include "common/utils/strings.h"
#include "http/command_exception.h"

namespace uh::cluster {

coro<void> directory::put_object(const std::string& bucket, const object& obj) {
    LOG_CORO_CONTEXT();
    if (!obj.addr) {
        throw std::runtime_error("put_object requires address");
    }

    auto data = to_buffer(*obj.addr);
    auto span = std::span<char>(data);

    try {
        LOG_DEBUG() << coro_id() << ": before m_db::get()";
        auto dir = co_await m_db.get();
        LOG_DEBUG() << coro_id() << ": after m_db::get()";
        co_await dir->execv("CALL uh_put_small_obj($1, $2, $3, $4, $5)", bucket,
                            obj.name, span, obj.addr->data_size(), obj.etag);
        LOG_DEBUG() << coro_id() << ": after execv, bucket: " << bucket
                    << ", obj: " << obj.name
                    << ", addr size: " << obj.addr->data_size();
    } catch (const std::exception& e) {
        throw command_exception(http::status::not_found, "NoSuchBucket",
                                "bucket not found");
    }
}

coro<object> directory::get_object(const std::string& bucket,
                                   const std::string& object_id) {
    LOG_CORO_CONTEXT();
    auto dir = co_await m_db.get();
    auto row = co_await dir->execb(
        "SELECT small::BYTEA, large FROM uh_get_object($1, $2)", bucket,
        object_id);

    if (!row) {
        throw command_exception(http::status::not_found, "NoSuchKey",
                                "object not found");
    }

    auto large = row->number(1);
    if (large) {
        throw std::runtime_error("large objects not supported");
    }

    auto small = row->data(0);
    if (!small) {
        throw std::runtime_error("small not defined");
    }

    address addr = to_address(*small);

    auto metadata = co_await dir->execv("SELECT size, last_modified, etag "
                                        "FROM uh_get_object($1, $2)",
                                        bucket, object_id);

    auto etag = metadata->string(2);

    co_return object{.name = object_id,
                     .last_modified = *metadata->date(1),
                     .size = static_cast<std::size_t>(*metadata->number(0)),
                     .addr = std::move(addr),
                     .etag = etag ? std::optional<std::string>(*etag)
                                  : std::nullopt};
}

coro<object> directory::head_object(const std::string& bucket,
                                    const std::string& object_id) {
    LOG_CORO_CONTEXT();
    auto dir = co_await m_db.get();
    auto metadata = co_await dir->execv("SELECT size, last_modified, etag "
                                        "FROM uh_get_object($1, $2)",
                                        bucket, object_id);

    if (!metadata) {
        throw command_exception(http::status::not_found, "NoSuchKey",
                                "object not found");
    }

    auto etag = metadata->string(2);

    co_return object{.name = object_id,
                     .last_modified = *metadata->date(1),
                     .size = static_cast<std::size_t>(*metadata->number(0)),
                     .addr = std::nullopt,
                     .etag = etag ? std::optional<std::string>(*etag)
                                  : std::nullopt};
}

coro<void> directory::put_bucket(const std::string& bucket) {
    LOG_CORO_CONTEXT();
    auto dir = co_await m_db.get();

    try {
        co_await dir->execv("CALL uh_create_bucket($1)", bucket);
    } catch (const std::exception&) {
        throw command_exception(http::status::conflict, "BucketAlreadyExists",
                                "The requested bucket name is not available.");
    }
}

coro<void> directory::bucket_exists(const std::string& bucket) {
    LOG_CORO_CONTEXT();
    auto dir = co_await m_db.get();

    try {
        co_await dir->execv("SELECT uh_bucket_exists($1)", bucket);
    } catch (const std::exception&) {
        throw error_exception(error::bucket_not_found);
    }
}

coro<void> directory::delete_bucket(const std::string& bucket) {
    LOG_CORO_CONTEXT();
    auto dir = co_await m_db.get();

    if (m_bucket_delete_policy == bucket_delete_policy::only_empty) {
        auto row = co_await dir->execv(
            "SELECT count(*) FROM uh_list_objects($1)", bucket);

        if (row->number(0) > 0) {
            throw command_exception(
                http::status::conflict, "BucketNotEmpty",
                "The bucket that you tried to delete is not empty.");
        }
    }

    co_await dir->execv("CALL uh_delete_bucket($1)", bucket);
}

coro<void> directory::delete_object(const std::string& bucket,
                                    const std::string& object_id) {
    LOG_CORO_CONTEXT();
    auto dir = co_await m_db.get();

    co_await dir->execv("CALL uh_delete_object($1, $2)", bucket, object_id);
}

coro<void> directory::copy_object(const std::string& bucket_src,
                                  const std::string& key_src,
                                  const std::string& bucket_dst,
                                  const std::string& key_dst) {
    LOG_CORO_CONTEXT();
    auto dir = co_await m_db.get();

    co_await dir->execv("CALL uh_copy_object($1, $2, $3, $4)", bucket_src,
                        key_src, bucket_dst, key_dst);
}

coro<void> directory::copy_object_ifmatch(const std::string& bucket_src,
                                          const std::string& key_src,
                                          const std::string& bucket_dst,
                                          const std::string& key_dst,
                                          const std::string& etag) {
    LOG_CORO_CONTEXT();
    auto dir = co_await m_db.get();
    co_await dir->execv("CALL uh_copy_object_ifmatch($1, $2, $3, $4, $5)",
                        bucket_src, key_src, bucket_dst, key_dst, etag);
}

coro<std::vector<std::string>> directory::list_buckets() {
    LOG_CORO_CONTEXT();
    std::vector<std::string> rv;

    auto dir = co_await m_db.get();

    for (auto row = co_await dir->exec("SELECT name FROM uh_list_buckets()");
         row; row = co_await dir->next()) {
        rv.emplace_back(*row->string(0));
    }

    co_return rv;
}

coro<std::vector<object>>
directory::list_objects(const std::string& bucket,
                        const std::optional<std::string>& prefix,
                        const std::optional<std::string>& lower_bound) {

    LOG_CORO_CONTEXT();
    auto dir = co_await m_db.get();
    std::vector<object> rv;

    auto row = co_await dir->execv("SELECT id, name, size, last_modified, "
                                   "etag FROM uh_list_objects($1, $2, $3)",
                                   bucket, prefix.value_or(""),
                                   lower_bound.value_or(""));
    for (; row; row = co_await dir->next()) {

        auto etag = row->string(4);
        rv.emplace_back(std::string(*row->string(1)), *row->date(3),
                        static_cast<std::size_t>(*row->number(2)), std::nullopt,
                        etag ? std::optional<std::string>(*etag)
                             : std::nullopt);
    }

    co_return rv;
}

coro<std::size_t> directory::data_size() {
    LOG_CORO_CONTEXT();
    std::size_t rv = 0;
    auto dir = co_await m_db.get();

    LOG_DEBUG() << "read directory data_size";

    auto buckets = co_await list_buckets();
    for (const auto& bucket : buckets) {
        auto row = co_await dir->execv("SELECT uh_bucket_size($1)", bucket);
        rv += *row->number(0);
    }

    LOG_DEBUG() << "read directory data_size: done";
    co_return rv;
}

} // namespace uh::cluster
