/*
 * Copyright 2026 UltiHash Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

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
    std::optional<std::string_view> string_view(int col);
    std::optional<std::string> string(int col);
    std::optional<std::span<char>> data(int col);
    std::optional<int64_t> number(int col);
    std::optional<std::size_t> size_type(int col);
    std::optional<utc_time> date(int col);
    std::optional<bool> boolean(int col);

private:
    friend class connection;
    row(std::shared_ptr<PGresult> result, int id);

    std::shared_ptr<PGresult> m_result;
    int m_row;
};

} // namespace uh::cluster::db
