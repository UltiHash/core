#ifndef CORE_COMMON_DB_RESULT_H
#define CORE_COMMON_DB_RESULT_H

#include "common/types/common_types.h"

#include <libpq-fe.h>

#include <memory>
#include <optional>
#include <span>
#include <string>

namespace uh::cluster::db {

class connection;

class result {
public:
    result(const result&) = delete;
    result(result&&) = default;

    PGresult* get() { return m_result.get(); }

    operator PGresult*() { return get(); }

    std::size_t rows() const;

    std::optional<std::string_view> string(int row, int col);
    std::optional<std::span<char>> data(int row, int col);
    std::optional<int64_t> number(int row, int col);
    std::optional<utc_time> date(int row, int col);

private:
    friend class connection;
    result(std::unique_ptr<PGresult, void (*)(PGresult*)>&& result)
        : m_result(std::move(result)) {}

    std::unique_ptr<PGresult, void (*)(PGresult*)> m_result;
};

} // namespace uh::cluster::db

#endif
