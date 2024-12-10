#ifndef CORE_COMMON_DB_CONNECTION_H
#define CORE_COMMON_DB_CONNECTION_H

#include "common/db/connstr.h"
#include "common/telemetry/log.h"
#include "common/utils/templates.h"
#include "row.h"
#include <boost/asio/posix/stream_descriptor.hpp>
#include <libpq-fe.h>
#include <memory>
#include <string>
#include <vector>

namespace uh::cluster::db {

class connection {
public:
    connection(boost::asio::io_context& ioc, const connstr& cs);
    connection(const connection&) = delete;
    connection(connection&&) = default;

    /**
     * Execute a query without parameters. The result format will be text.
     */
    coro<std::optional<row>> exec(const std::string& query);

    /**
     * Execute a query synchronously and return the first row. You can use
     * `next()` to retrieve subsequent rows, though this requires coroutine
     * context.
     */
    std::optional<row> raw_exec(const std::string& query);

    /**
     * Execute a query with parameter variables passing the variable values
     * as parameter pack. The query result will be returned as textual value.
     */
    template <typename... args>
    coro<std::optional<row>> execv(const std::string& query, args... a) {
        return exec_format(query, 0, a...);
    }

    /**
     * Execute a query with parameter variables passing the variable values
     * as parameter pack *synchronously*. The query result will be returned
     * as textual value.
     */
    template <typename... args>
    std::optional<row> raw_execv(const std::string& query, args... a) {
        return raw_exec_format(query, 0, a...);
    }

    /**
     * Execute a query with parameter variables passing the variable values
     * as parameter pack. The query result will be returned as binary value.
     */
    template <typename... args>
    coro<std::optional<row>> execb(const std::string& query, args... a) {
        return exec_format(query, 1, a...);
    }

    /**
     * Return the next row for the current command. Returns std::nullopt if
     * there are no more rows to retrieve.
     */
    coro<std::optional<row>> next();

    /**
     * Cancel the current command.
     */
    coro<void> cancel();

    std::string id() const;

private:
    template <typename... args>
    coro<std::optional<row>> exec_format(const std::string& query,
                                         int result_format, args... a) {
        std::vector<const char*> values;
        std::vector<int> lengths;
        std::vector<int> format;
        std::list<std::string> memory;

        foreach (
            [&](const auto& h) {
                append_args(h, values, lengths, format, memory);
            },
            a...)
            ;

        LOG_DEBUG() << id() << ": exec_format(" << query << ")";
        co_await cancel();
        if (!PQsendQueryParams(m_ptr.get(), query.c_str(), sizeof...(a),
                               nullptr, values.data(), lengths.data(),
                               format.data(), result_format)) {
            throw_error_message();
        }

        co_return co_await next();
    }

    template <typename... args>
    std::optional<row> raw_exec_format(const std::string& query,
                                       int result_format, args... a) {
        std::vector<const char*> values;
        std::vector<int> lengths;
        std::vector<int> format;
        std::list<std::string> memory;

        foreach (
            [&](const auto& h) {
                append_args(h, values, lengths, format, memory);
            },
            a...)
            ;

        LOG_DEBUG() << id() << ": raw_exec_format(" << query << ")";
        m_result = std::shared_ptr<PGresult>(
            PQexecParams(m_ptr.get(), query.c_str(), sizeof...(a), nullptr,
                         values.data(), lengths.data(), format.data(),
                         result_format),
            PQclear);
        m_row = 0;
        if (!m_result) {
            throw_error_message();
        }

        if (PQntuples(m_result.get()) == 0) {
            m_result.reset();
            return std::nullopt;
        }

        return row(m_result, 0);
    }

    coro<void> wait();

    [[noreturn]] void throw_error_message();

    void append_args(std::span<char> s, std::vector<const char*>& values,
                     std::vector<int>& lengths, std::vector<int>& format,
                     std::list<std::string>&);

    void append_args(std::string s, std::vector<const char*>& values,
                     std::vector<int>& lengths, std::vector<int>& format,
                     std::list<std::string>& mem);

    void append_args(std::string_view s, std::vector<const char*>& values,
                     std::vector<int>& lengths, std::vector<int>& format,
                     std::list<std::string>&);

    void append_args(std::size_t n, std::vector<const char*>& values,
                     std::vector<int>& lengths, std::vector<int>& format,
                     std::list<std::string>& mem);

    template <typename type>
    void append_args(const std::optional<type>& value,
                     std::vector<const char*>& values,
                     std::vector<int>& lengths, std::vector<int>& format,
                     std::list<std::string>& mem) {
        if (value) {
            append_args(*value, values, lengths, format, mem);
        } else {
            values.push_back(nullptr);
            lengths.push_back(0);
            format.push_back(0);
        }
    }

    boost::asio::io_context& m_ioc;
    std::unique_ptr<PGconn, void (*)(PGconn*)> m_ptr;
    boost::asio::posix::stream_descriptor m_fd;

    std::shared_ptr<PGresult> m_result;
    int m_row = 0;
};

} // namespace uh::cluster::db

#endif
