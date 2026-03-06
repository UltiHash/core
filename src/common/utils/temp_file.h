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

#include <filesystem>
#include <string_view>
#include <unistd.h>

class temp_file {
public:
    explicit temp_file(const std::filesystem::path& directory)
        : m_path(open_temp_file(directory)) {}

    [[nodiscard]] inline auto get_path() const noexcept { return m_path; }

    inline void release_to(const std::filesystem::path& path) {
        std::filesystem::rename(m_path, path);
        m_remove = false;
    }
    ~temp_file() {
        if (m_remove) {
            std::filesystem::remove(m_path);
        }
    }

private:
    static std::filesystem::path
    open_temp_file(const std::filesystem::path& temp_dir) {
        auto path = (temp_dir / FILENAME_TEMPLATE).string();

        int fd = mkstemp(path.data());
        if (fd == -1) {
            throw std::runtime_error("Could not create a temporary file");
        }
        close(fd);
        return path;
    }

    std::filesystem::path m_path;
    bool m_remove = true;

    constexpr static std::string_view FILENAME_TEMPLATE = "tempfile-XXXXXX";
};
