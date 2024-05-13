#ifndef CORE_COMMON_DB_CONNECTION_H
#define CORE_COMMON_DB_CONNECTION_H

#include "common/telemetry/log.h"
#include "common/types/scoped_buffer.h"
#include "common/utils/templates.h"
#include "result.h"
#include <libpq-fe.h>
#include <memory>
#include <string>
#include <vector>

#include <signal.h>

namespace uh::cluster::db {

class connection {
public:
    connection(const std::string& connstr);
    connection(const connection&) = delete;
    connection(connection&&) = default;

    /**
     * Execute a query without parameters. The result format will be text.
     */
    result exec(const std::string& query);

    /**
     * Execute a query with parameter variables passing the variable values
     * as parameter pack. The query result will be returned as textual value.
     */
    template <typename... args>
    result execv(const std::string& query, args... a) {
        std::vector<const char*> values;
        std::vector<int> lengths;
        std::vector<int> format;
        std::list<unique_buffer<>> memory;

        foreach (
            [&](const auto& h) {
                append_args(h, values, lengths, format, memory);
            },
            a...)
            ;

        LOG_DEBUG() << "execv: \"" << query << "\"";

        auto res = std::unique_ptr<PGresult, void (*)(PGresult*)>(
            PQexecParams(m_ptr.get(), query.c_str(), sizeof...(a), nullptr,
                         values.data(), lengths.data(), format.data(), 0),
            PQclear);

        check_result(res.get());

        return result(std::move(res));
    }

    /**
     * Execute a query with parameter variables passing the variable values
     * as parameter pack. The query result will be returned as binary value.
     */
    template <typename... args>
    result execb(const std::string& query, args... a) {
        std::vector<const char*> values;
        std::vector<int> lengths;
        std::vector<int> format;
        std::list<unique_buffer<>> memory;

        foreach (
            [&](const auto& h) {
                append_args(h, values, lengths, format, memory);
            },
            a...)
            ;

        LOG_DEBUG() << "execv: \"" << query << "\"";

        auto res = std::unique_ptr<PGresult, void (*)(PGresult*)>(
            PQexecParams(m_ptr.get(), query.c_str(), sizeof...(a), nullptr,
                         values.data(), lengths.data(), format.data(), 1),
            PQclear);

        check_result(res.get());

        return result(std::move(res));
    }

private:
    void append_args(std::span<char> s, std::vector<const char*>& values,
                     std::vector<int>& lengths, std::vector<int>& format,
                     std::list<unique_buffer<>>&) {
        values.push_back(s.data());
        lengths.push_back(s.size());
        format.push_back(1);
    }

    void append_args(std::string_view s, std::vector<const char*>& values,
                     std::vector<int>& lengths, std::vector<int>& format,
                     std::list<unique_buffer<>>&) {
        values.push_back(s.data());
        lengths.push_back(s.size());
        format.push_back(0);
    }

    void append_args(std::size_t n, std::vector<const char*>& values,
                     std::vector<int>& lengths, std::vector<int>& format,
                     std::list<unique_buffer<>>& mem) {
        auto s = std::to_string(n);
        auto& buffer = mem.emplace_back(s.size());
        memcpy(buffer.data(), s.data(), s.size());

        values.push_back(buffer.data());
        lengths.push_back(s.size());
        format.push_back(0);
    }

    void check_result(const PGresult* result);

    std::unique_ptr<PGconn, void (*)(PGconn*)> m_ptr;
};

} // namespace uh::cluster::db

#endif
