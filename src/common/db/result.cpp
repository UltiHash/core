#include "result.h"
#include <charconv>

namespace uh::cluster::db {

std::size_t result::rows() const { return PQntuples(m_result.get()); }

std::optional<std::span<char>> result::data(int row, int col) {
    if (PQgetisnull(m_result.get(), row, col)) {
        return {};
    }

    char* data = PQgetvalue(m_result.get(), row, col);
    int len = PQgetlength(m_result.get(), row, col);

    return std::span(data, len);
}

std::optional<std::string_view> result::string(int row, int col) {
    if (PQgetisnull(m_result.get(), row, col)) {
        return {};
    }

    char* data = PQgetvalue(m_result.get(), row, col);
    int len = PQgetlength(m_result.get(), row, col);

    return std::string_view(data, len);
}

std::optional<int64_t> result::number(int row, int col) {
    if (PQgetisnull(m_result.get(), row, col)) {
        return {};
    }

    char* data = PQgetvalue(m_result.get(), row, col);
    int len = PQgetlength(m_result.get(), row, col);

    int64_t result{};
    auto [ptr, ec] = std::from_chars(data, data + len, result);
    if (ec != std::errc()) {
        throw std::runtime_error("from_chars failed: " +
                                 std::make_error_condition(ec).message());
    }

    return result;
}

std::optional<utc_time> result::date(int row, int col) {
    if (PQgetisnull(m_result.get(), row, col)) {
        return {};
    }

    char* data = PQgetvalue(m_result.get(), row, col);
    std::stringstream in(data);

    std::tm tm;
    in >> std::get_time(&tm, "%Y-%m-%d %H-%M-%S");

    return utc_time::clock::from_time_t(std::mktime(&tm));
}

} // namespace uh::cluster::db
