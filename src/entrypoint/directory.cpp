#include "directory.h"
#include "common/utils/strings.h"
#include "http/command_exception.h"

using namespace uh::cluster::ep::http;

namespace uh::cluster {

std::string to_string(bucket_versioning versioning) {
    switch (versioning) {
        case bucket_versioning::disabled: return "Disabled";
        case bucket_versioning::enabled: return "Enabled";
        case bucket_versioning::suspended: return "Suspended";
    }

    throw std::runtime_error("unsupported versioning type");
}

bucket_versioning to_versioning(std::string s) {
    if (equals_nocase(s, "Disabled")) {
        return bucket_versioning::disabled;
    }
    if (equals_nocase(s, "Enabled")) {
        return bucket_versioning::enabled;
    }
    if (equals_nocase(s, "Suspended")) {
        return bucket_versioning::suspended;
    }

    throw std::runtime_error("unsupported versioning type: " + s);
}

coro<void> directory::put_object(const std::string& bucket, const object& obj) {
    if (!obj.addr) {
        throw std::runtime_error("put_object requires address");
    }

    auto data = to_buffer(*obj.addr);
    auto span = std::span<char>(data);

    auto handle = co_await m_db.get();
    try {
        co_await handle->execv("CALL uh_put_object($1, $2, $3, $4, $5, $6)",
                               bucket, obj.name, span, obj.addr->data_size(),
                               obj.etag, obj.mime);
    } catch (const std::exception& e) {
        throw command_exception(status::not_found, "NoSuchBucket",
                                "The specified bucket does not exist.");
    }
}

coro<directory::object_lock>
directory::get_object(const std::string& bucket, const std::string& object_id) {
    auto handle = co_await m_db.get();
    auto row = co_await handle->execb(
        "SELECT address::BYTEA FROM uh_get_object($1, $2)", bucket, object_id);

    if (!row) {
        throw command_exception(status::not_found, "NoSuchKey",
                                "The specified key does not exist.");
    }

    auto addr_data = row->data(0);
    if (!addr_data) {
        throw std::runtime_error("address data not defined");
    }

    address addr = to_address(*addr_data);

    auto metadata =
        co_await handle->execv("SELECT size, last_modified, etag, mime, id "
                               "FROM uh_get_object($1, $2)",
                               bucket, object_id);

    std::size_t id = *metadata->number(4);

    co_await handle->execv("CALL uh_inc_reference($1)", id);

    auto executor = co_await boost::asio::this_coro::executor;
    promise<void> p;
    future<void> f = p.get_future();

    boost::asio::co_spawn(
        executor,
        [f = std::move(f), this, id]() mutable -> coro<void> {
            co_await f.get();
            auto h = co_await m_db.get();
            co_await h->execv("CALL uh_dec_reference($1)", id);
        },
        boost::asio::detached);

    co_return object_lock(object{.name = object_id,
                                 .last_modified = *metadata->date(1),
                                 .size = *metadata->size_type(0),
                                 .addr = std::move(addr),
                                 .etag = metadata->string(2),
                                 .mime = metadata->string(3)},
                          unref{std::move(p)});
}

void directory::unref::operator()() { p.set_value(); }

coro<object> directory::head_object(const std::string& bucket,
                                    const std::string& object_id) {
    auto handle = co_await m_db.get();
    auto metadata = co_await handle->execv(
        "SELECT size, last_modified, etag, mime FROM uh_get_object($1, $2)",
        bucket, object_id);

    if (!metadata) {
        throw command_exception(status::not_found, "NoSuchKey",
                                "The specified key does not exist.");
    }

    co_return object{.name = object_id,
                     .last_modified = *metadata->date(1),
                     .size = *metadata->size_type(0),
                     .addr = std::nullopt,
                     .etag = metadata->string(2),
                     .mime = metadata->string(3)};
}

coro<void> directory::put_bucket(const std::string& bucket) {
    LOG_DEBUG() << "put_bucket(" << bucket << ")";
    validate_bucket_name(bucket);

    try {
        auto handle = co_await m_db.get();
        co_await handle->execv("CALL uh_create_bucket($1)", bucket);
    } catch (const std::exception&) {
        throw command_exception(
            status::conflict, "BucketAlreadyExists",
            "The requested bucket name is not available. The bucket namespace "
            "is shared by all users of the system. Specify a different name "
            "and try again.");
    }
}

coro<void> directory::bucket_exists(const std::string& bucket) {

    try {
        auto handle = co_await m_db.get();
        co_await handle->execv("SELECT uh_bucket_exists($1)", bucket);
    } catch (const std::exception&) {
        throw command_exception(status::not_found, "NoSuchBucket",
                                "The specified bucket does not exist.");
    }
}

coro<void> directory::delete_bucket(const std::string& bucket) {
    co_await bucket_exists(bucket);

    auto handle = co_await m_db.get();
    auto row = co_await handle->execv(
        "SELECT count(*) FROM uh_list_objects($1)", bucket);

    if (row->number(0) > 0) {
        throw command_exception(
            status::conflict, "BucketNotEmpty",
            "The bucket that you tried to delete is not empty.");
    }

    co_await handle->execv("CALL uh_delete_bucket($1)", bucket);
}

coro<void> directory::delete_object(const std::string& bucket,
                                    const std::string& object_id) {

    try {
        auto handle = co_await m_db.get();
        co_await handle->execv("CALL uh_delete_object($1, $2)", bucket,
                               object_id);
    } catch (const std::exception& e) {
    }
}

coro<std::vector<std::string>> directory::list_buckets() {
    std::vector<std::string> rv;

    auto handle = co_await m_db.get();
    for (auto row = co_await handle->exec("SELECT name FROM uh_list_buckets()");
         row; row = co_await handle->next()) {
        rv.emplace_back(*row->string(0));
    }

    co_return rv;
}

coro<std::optional<std::string>>
directory::get_bucket_policy(const std::string& bucket) {

    try {
        auto handle = co_await m_db.get();
        auto row = co_await handle->execv(
            "SELECT policy FROM uh_bucket_policy($1)", bucket);
        co_return row->string(0);
    } catch (const std::exception& e) {
        throw command_exception(status::not_found, "NoSuchBucket",
                                "The specified bucket does not exist.");
    }

    co_return std::nullopt;
}

coro<void> directory::set_bucket_policy(const std::string& bucket,
                                        std::optional<std::string> policy) {
    co_await bucket_exists(bucket);

    auto handle = co_await m_db.get();
    co_await handle->execv("CALL uh_bucket_set_policy($1, $2)", bucket, policy);
}

coro<std::optional<std::string>>
directory::get_bucket_cors(const std::string& bucket) {

    try {
        auto handle = co_await m_db.get();
        auto row = co_await handle->execv("SELECT cors FROM uh_bucket_cors($1)",
                                          bucket);
        co_return row->string(0);
    } catch (const std::exception& e) {
        throw command_exception(status::not_found, "NoSuchBucket",
                                "The specified bucket does not exist.");
    }

    co_return std::nullopt;
}

coro<void> directory::set_bucket_cors(const std::string& bucket,
                                      std::optional<std::string> cors) {
    co_await bucket_exists(bucket);

    auto handle = co_await m_db.get();
    co_await handle->execv("CALL uh_bucket_set_cors($1, $2)", bucket, cors);
}

coro<bucket_versioning> directory::get_bucket_versioning(const std::string& bucket) {
    try {
        auto handle = co_await m_db.get();
        auto row = co_await handle->execv("SELECT status FROM uh_bucket_versioning($1)", bucket);

        co_return to_versioning(*row->string(0));
    } catch (const std::exception& e) {
        throw command_exception(status::not_found, "NoSuchBucket",
                                "The specified bucket does not exist.");
    }
}

coro<void> directory::set_bucket_versioning(const std::string& bucket,
        bucket_versioning versioning) {
    co_await bucket_exists(bucket);

    auto handle = co_await m_db.get();
    co_await handle->execv("CALL uh_bucket_set_versioning($1, $2)", bucket, to_string(versioning));
}

coro<std::vector<object>>
directory::list_objects(const std::string& bucket,
                        const std::optional<std::string>& prefix,
                        const std::optional<std::string>& lower_bound) {
    co_await bucket_exists(bucket);

    std::vector<object> rv;

    auto handle = co_await m_db.get();
    auto row = co_await handle->execv(
        "SELECT id, name, size, last_modified, "
        "etag, mime FROM uh_list_objects($1, $2, $3)",
        bucket, prefix.value_or(""), lower_bound.value_or(""));
    for (; row; row = co_await handle->next()) {

        rv.emplace_back(object{.name = *row->string(1),
                               .last_modified = *row->date(3),
                               .size = *row->size_type(2),
                               .addr = std::nullopt,
                               .etag = row->string(4),
                               .mime = row->string(5)});
    }

    co_return rv;
}

coro<std::optional<directory::to_delete>> directory::next_deleted() {
    auto handle = co_await m_db.get();

    auto row = co_await handle->execb(
        "SELECT id, address FROM uh_next_deleted() LIMIT 1");
    if (!row) {
        co_return std::nullopt;
    }

    to_delete rv;
    rv.id = *row->number(0);
    rv.addr = to_address(*row->data(1));

    co_return rv;
}

coro<void> directory::clear_buckets() {
    auto handle = co_await m_db.get();
    co_await handle->exec("CALL uh_clear_deleted_buckets();");
}

coro<void> directory::remove_object(std::size_t id) {
    auto handle = co_await m_db.get();

    co_await handle->execv("CALL uh_delete_object_by_id($1)", id);
}

coro<std::size_t> directory::data_size() {
    auto handle = co_await m_db.get();
    auto row = co_await handle->execv("SELECT uh_data_size()");
    co_return row->number(0).value_or(0);
}

void directory::validate_bucket_name(const std::string& bucket_name) {
    if (bucket_name.size() < 3 || bucket_name.size() > 63) {
        throw command_exception(
            status::bad_request, "InvalidBucketName",
            "The specified bucket name has invalid length.");
    }

    std::regex bucket_pattern(
        R"(^(?!(xn--|sthree-|sthree-configurator-))(?!.*-s3alias$)(?!.*--ol-s3$)(?!^(\d{1,3}\.){3}\d{1,3}$)[a-z0-9](?!.*\.\.)(?!.*[.\s-][.\s-])[a-z0-9.-]*[a-z0-9]$)");
    if (!std::regex_match(bucket_name, bucket_pattern)) {
        throw command_exception(
            status::bad_request, "InvalidBucketName",
            "The specified bucket name has invalid characters.");
    }
}

coro<void> safe_put_object(directory& dir,
                           storage::global::global_data_view& gdv,
                           const std::string& bucket, const object& obj) {
    std::optional<std::exception_ptr> error;
    try {
        co_await dir.put_object(bucket, obj);
    } catch (...) {
        error = std::current_exception();
    }

    if (error) {
        co_await gdv.unlink(*obj.addr);
        std::rethrow_exception(*error);
    }
}

} // namespace uh::cluster
