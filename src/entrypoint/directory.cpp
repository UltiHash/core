#include "directory.h"
#include "common/utils/strings.h"
#include "http/command_exception.h"

namespace uh::cluster {

coro<void> directory::put_object(const std::string& bucket, const object& obj) {
    if (!obj.addr) {
        throw std::runtime_error("put_object requires address");
    }

    auto data = to_buffer(*obj.addr);
    auto span = std::span<char>(data);

    try {
        auto dir = co_await m_db.get();
        co_await dir->execv("CALL uh_put_small_obj($1, $2, $3, $4, $5, $6)",
                            bucket, obj.name, span, obj.addr->data_size(),
                            obj.etag, obj.mime);
    } catch (const std::exception& e) {
        throw command_exception(http::status::not_found, "NoSuchBucket",
                                "bucket not found");
    }
}

coro<object> directory::get_object(const std::string& bucket,
                                   const std::string& object_id) {
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

    auto metadata =
        co_await dir->execv("SELECT size, last_modified, etag, mime "
                            "FROM uh_get_object($1, $2)",
                            bucket, object_id);

    co_return object{.name = object_id,
                     .last_modified = *metadata->date(1),
                     .size = *metadata->size_type(0),
                     .addr = std::move(addr),
                     .etag = metadata->string(2),
                     .mime = metadata->string(3)};
}

coro<object> directory::head_object(const std::string& bucket,
                                    const std::string& object_id) {
    auto dir = co_await m_db.get();
    auto metadata =
        co_await dir->execv("SELECT size, last_modified, etag, mime "
                            "FROM uh_get_object($1, $2)",
                            bucket, object_id);

    if (!metadata) {
        throw command_exception(http::status::not_found, "NoSuchKey",
                                "object not found");
    }

    co_return object{.name = object_id,
                     .last_modified = *metadata->date(1),
                     .size = *metadata->size_type(0),
                     .addr = std::nullopt,
                     .etag = metadata->string(2),
                     .mime = metadata->string(3)};
}

coro<void> directory::put_bucket(const std::string& bucket) {
    validate_bucket_name(bucket);

    auto dir = co_await m_db.get();

    try {
        co_await dir->execv("CALL uh_create_bucket($1)", bucket);
    } catch (const std::exception&) {
        throw command_exception(http::status::conflict, "BucketAlreadyExists",
                                "The requested bucket name is not available.");
    }
}

coro<void> directory::bucket_exists(const std::string& bucket) {
    auto dir = co_await m_db.get();

    try {
        co_await dir->execv("SELECT uh_bucket_exists($1)", bucket);
    } catch (const std::exception&) {
        throw error_exception(error::bucket_not_found);
    }
}

coro<void> directory::delete_bucket(const std::string& bucket) {
    co_await bucket_exists(bucket);

    auto dir = co_await m_db.get();

    auto row =
        co_await dir->execv("SELECT count(*) FROM uh_list_objects($1)", bucket);

    if (row->number(0) > 0) {
        throw command_exception(
            http::status::conflict, "BucketNotEmpty",
            "The bucket that you tried to delete is not empty.");
    }

    co_await dir->execv("CALL uh_delete_bucket($1)", bucket);
}

coro<void> directory::delete_object(const std::string& bucket,
                                    const std::string& object_id) {
    auto dir = co_await m_db.get();

    co_await dir->execv("CALL uh_delete_object($1, $2)", bucket, object_id);
}

coro<void> directory::copy_object(const std::string& bucket_src,
                                  const std::string& key_src,
                                  const std::string& bucket_dst,
                                  const std::string& key_dst) {
    auto dir = co_await m_db.get();

    co_await dir->execv("CALL uh_copy_object($1, $2, $3, $4)", bucket_src,
                        key_src, bucket_dst, key_dst);
}

coro<std::vector<std::string>> directory::list_buckets() {
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
    co_await bucket_exists(bucket);

    auto dir = co_await m_db.get();
    std::vector<object> rv;

    auto row = co_await dir->execv(
        "SELECT id, name, size, last_modified, "
        "etag, mime FROM uh_list_objects($1, $2, $3)",
        bucket, prefix.value_or(""), lower_bound.value_or(""));
    for (; row; row = co_await dir->next()) {

        rv.emplace_back(object{.name = *row->string(1),
                               .last_modified = *row->date(3),
                               .size = *row->size_type(2),
                               .addr = std::nullopt,
                               .etag = row->string(4),
                               .mime = row->string(5)});
    }

    co_return rv;
}

coro<std::size_t> directory::data_size() {
    std::size_t rv = 0;
    auto dir = co_await m_db.get();

    LOG_DEBUG() << "read directory data_size";

    auto buckets = co_await list_buckets();
    for (const auto& bucket : buckets) {
        auto row = co_await dir->execv("SELECT uh_bucket_size($1)", bucket);
        rv += row->number(0).value_or(0);
    }

    LOG_DEBUG() << "read directory data_size: done";
    co_return rv;
}

void directory::validate_bucket_name(const std::string& bucket_name) {
    if (bucket_name.size() < 3 || bucket_name.size() > 63) {
        throw command_exception(http::status::bad_request, "InvalidBucketName",
                                "bucket name has invalid length");
    }

    std::regex bucket_pattern(
        R"(^(?!(xn--|sthree-|sthree-configurator-))(?!.*-s3alias$)(?!.*--ol-s3$)(?!^(\d{1,3}\.){3}\d{1,3}$)[a-z0-9](?!.*\.\.)(?!.*[.\s-][.\s-])[a-z0-9.-]*[a-z0-9]$)");
    if (!std::regex_match(bucket_name, bucket_pattern)) {
        throw command_exception(http::status::bad_request, "InvalidBucketName",
                                "bucket name has invalid characters");
    }
}

} // namespace uh::cluster
