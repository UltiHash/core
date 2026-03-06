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

#include "body.h"
#include "raw_request.h"
#include <common/utils/common.h>
#include <entrypoint/http/stream.h>

#include <map>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace uh::cluster::ep::http {

class chunked_body : public ep::http::body {
public:
    enum class trailing_headers { none, read };

    chunked_body(stream& s, trailing_headers trailing = trailing_headers::none);

    struct chunk_header {
        std::size_t size;
        std::map<std::string_view, std::string_view> extensions;
        std::string extensions_string;
    };

    std::optional<std::size_t> length() const override;
    coro<std::span<const char>> read(std::size_t count) override;
    coro<void> consume() override;
    std::size_t buffer_size() const override;

    virtual void on_chunk_header(const chunk_header&);
    virtual void on_chunk_data(std::span<const char>);
    virtual void on_chunk_done();
    virtual void on_body_done();

private:
    coro<chunk_header> read_chunk_header();

    static constexpr std::size_t BUFFER_SIZE = MEBI_BYTE;

    stream& m_s;
    trailing_headers m_trailing;
    std::size_t m_chunk_bytes_left = 0ull;
    bool m_end = false;
};

} // namespace uh::cluster::ep::http
