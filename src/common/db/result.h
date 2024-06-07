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

class row {
public:
    std::optional<std::string_view> string(int col);
    std::optional<std::span<char>> data(int col);
    std::optional<int64_t> number(int col);
    std::optional<utc_time> date(int col);

private:
    friend class connection;
    row(std::shared_ptr<PGresult> result, int id);

    std::shared_ptr<PGresult> m_result;
    int m_row;
};

} // namespace uh::cluster::db

#endif
