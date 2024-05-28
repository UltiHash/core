#include "directory.h"

#include "common/utils/strings.h"
#include "http/command_exception.h"

namespace uh::cluster {

coro<void> directory::put_object(const std::string& bucket,
                                 const std::string& object_id,
                                 const address& addr) {
    std::vector<char> data;
    zpp::bits::out{data, zpp::bits::size4b{}}(addr).or_throw();
    auto span = std::span<char>(data);

    try {
        m_db.directory()->execv("CALL uh_put_small_obj($1, $2, $3, $4)", bucket,
                                object_id, span, addr.data_size());
    } catch (const std::exception& e) {
        throw command_exception(http::status::not_found, "NoSuchBucket",
                                "bucket not found");
    }

    co_return;
}

coro<object> directory::get_object(const std::string& bucket,
                                   const std::string& object_id) {
    auto res = m_db.directory()->execb(
        "SELECT small::BYTEA, large FROM uh_get_object($1, $2)", bucket,
        object_id);

    if (res.rows() == 0) {
        throw command_exception(http::status::not_found, "NoSuchKey",
                                "object not found");
    }

    auto large = res.number(0, 1);
    if (large) {
        throw std::runtime_error("large objects not supported");
    }

    auto small = res.data(0, 0);
    if (!small) {
        throw std::runtime_error("small not defined");
    }

    address addr;
    zpp::bits::in{*small, zpp::bits::size4b{}}(addr).or_throw();

    auto metadata = m_db.directory()->execv(
        "SELECT size, last_modified FROM uh_get_object($1, $2)", bucket,
        object_id);

    co_return object{.name = object_id,
                     .last_modified = *metadata.date(0, 1),
                     .size = static_cast<std::size_t>(*metadata.number(0, 0)),
                     .addr = std::move(addr)};
}

coro<object> directory::head_object(const std::string& bucket,
                                    const std::string& object_id) {
    auto metadata = m_db.directory()->execv(
        "SELECT size, last_modified FROM uh_get_object($1, $2)", bucket,
        object_id);

    if (metadata.rows() == 0) {
        throw command_exception(http::status::not_found, "NoSuchKey",
                                "object not found");
    }

    co_return object{.name = object_id,
                     .last_modified = *metadata.date(0, 1),
                     .size = static_cast<std::size_t>(*metadata.number(0, 0)),
                     .addr = std::nullopt};
}

coro<void> directory::put_bucket(const std::string& bucket) {
    try {
        m_db.directory()->execv("CALL uh_create_bucket($1)", bucket);
    } catch (const std::exception&) {
        throw command_exception(http::status::conflict, "BucketAlreadyExists",
                                "The requested bucket name is not available.");
    }
    co_return;
}

coro<void> directory::bucket_exists(const std::string& bucket) {
    try {
        m_db.directory()->execv("SELECT uh_bucket_exists($1)", bucket);
    } catch (const std::exception&) {
        throw error_exception(error::bucket_not_found);
    }
    co_return;
}

coro<void> directory::delete_bucket(const std::string& bucket) {

    if (m_bucket_delete_policy == bucket_delete_policy::only_empty) {
        auto res = m_db.directory()->execv(
            "SELECT count(*) FROM uh_list_objects($1)", bucket);

        if (res.number(0, 0) > 0) {
            throw command_exception(
                http::status::conflict, "BucketNotEmpty",
                "The bucket that you tried to delete is not empty.");
        }
    }

    m_db.directory()->execv("CALL uh_delete_bucket($1)", bucket);
    co_return;
}

coro<void> directory::delete_object(const std::string& bucket,
                                    const std::string& object_id) {
    m_db.directory()->execv("CALL uh_delete_object($1, $2)", bucket, object_id);
    co_return;
}

coro<std::vector<std::string>> directory::list_buckets() {
    std::vector<std::string> rv;

    auto res = m_db.directory()->exec("SELECT name FROM uh_list_buckets()");

    for (auto row = 0ull; row < res.rows(); ++row) {
        rv.emplace_back(*res.string(row, 0));
    }

    co_return rv;
}

coro<std::vector<object>>
directory::list_objects(const std::string& bucket,
                        const std::optional<std::string>& prefix,
                        const std::optional<std::string>& lower_bound) {

    auto res = m_db.directory()->execv(
        "SELECT id, name, size, last_modified FROM uh_list_objects($1, $2, $3)",
        bucket, prefix.value_or(""), lower_bound.value_or(""));

    std::vector<object> rv;
    rv.reserve(res.rows());
    for (auto row = 0ull; row < res.rows(); ++row) {
        rv.emplace_back(std::string(*res.string(row, 1)), *res.date(row, 3),
                        static_cast<std::size_t>(*res.number(row, 2)));
    }

    co_return rv;
}

coro<std::size_t> directory::data_size() {
    std::size_t rv = 0;

    auto buckets = co_await list_buckets();
    for (const auto& bucket : buckets) {
        auto result =
            m_db.directory()->execv("SELECT uh_bucket_size($1)", bucket);
        rv += *result.number(0, 0);
    }

    co_return rv;
}

} // namespace uh::cluster
