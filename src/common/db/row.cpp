#include "row.h"
#include "common/telemetry/log.h"
#include "common/utils/strings.h"
#include <charconv>
#include <endian.h>

namespace uh::cluster::db {

row::row(std::shared_ptr<PGresult> result, int id)
    : m_result(result),
      m_row(id) {}

std::optional<std::span<char>> row::data(int col) {
    if (PQgetisnull(m_result.get(), m_row, col)) {
        return {};
    }

    char* data = PQgetvalue(m_result.get(), m_row, col);
    int len = PQgetlength(m_result.get(), m_row, col);

    return std::span(data, len);
}

std::optional<std::string_view> row::string_view(int col) {
    if (PQgetisnull(m_result.get(), m_row, col)) {
        return {};
    }

    char* data = PQgetvalue(m_result.get(), m_row, col);
    int len = PQgetlength(m_result.get(), m_row, col);

    return std::string_view(data, len);
}

std::optional<std::string> row::string(int col) {
    if (PQgetisnull(m_result.get(), m_row, col)) {
        return {};
    }

    char* data = PQgetvalue(m_result.get(), m_row, col);
    int len = PQgetlength(m_result.get(), m_row, col);

    return std::string(data, len);
}

std::optional<int64_t> row::number(int col) {
    if (PQgetisnull(m_result.get(), m_row, col)) {
        return {};
    }

    char* data = PQgetvalue(m_result.get(), m_row, col);
    int len = PQgetlength(m_result.get(), m_row, col);

    int64_t result{};
    switch (PQfformat(m_result.get(), col)) {
    case 0: {
        auto [ptr, ec] = std::from_chars(data, data + len, result);
        if (ec != std::errc()) {
            throw std::runtime_error(
                "from_chars '" + std::string(data, len) +
                "' failed: " + std::make_error_condition(ec).message());
        }
    }; break;

    case 1: {
        if (sizeof(result) != len) {
            throw std::runtime_error("size mismatch reading binary number");
        }

        memcpy(&result, data, sizeof(result));
        result = be64toh(result);
    }; break;

    default:
        throw std::runtime_error("unsupported format reference");
    }

    return result;
}

std::optional<std::size_t> row::size_type(int col) {
    auto rv = number(col);
    if (!rv) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(*rv);
}

std::optional<utc_time> row::date(int col) {
    if (PQgetisnull(m_result.get(), m_row, col)) {
        return {};
    }

    if (PQfformat(m_result.get(), col) == 1) {
        throw std::runtime_error("unsupported date format");
    }

    char* data = PQgetvalue(m_result.get(), m_row, col);
    std::stringstream in(data);

    std::tm tm{};
    in >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

    return utc_time::clock::from_time_t(std::mktime(&tm));
}

std::optional<bool> row::boolean(int col) {
    auto rv = number(col);
    if (!rv) {
        return std::nullopt;
    }

    return rv != 0;
}

} // namespace uh::cluster::db
