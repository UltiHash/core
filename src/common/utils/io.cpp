// Copyright 2026 UltiHash Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "io.h"
#include "error.h"
#include <fcntl.h>
#include <unistd.h>

namespace uh::cluster {

std::size_t safe_pread(int fd, std::span<char> buffer, std::size_t offset) {

    std::size_t read = 0ull;

    while (read < buffer.size()) {
        auto rc = ::pread(fd, buffer.data() + read, buffer.size() - read,
                          offset + read);
        if (rc == -1) {
            throw_from_errno("pread failed");
        }

        read += rc;
    }

    return read;
}

std::size_t safe_pwrite(int fd, std::span<const char> buffer,
                        std::size_t offset) {

    std::size_t written = 0ull;

    while (written < buffer.size()) {
        auto rc = ::pwrite(fd, buffer.data() + written, buffer.size() - written,
                           offset + written);
        if (rc == -1) {
            throw_from_errno("pwrite failed");
        }

        written += rc;
    }

    return written;
}

std::filesystem::path operator+(const std::filesystem::path& p, std::string s) {
    auto rv = p;
    rv += s;
    return rv;
}

int open_file(const std::filesystem::path& path) {
    int fd = ::open(path.c_str(), O_RDWR);
    if (fd == -1) {
        throw_from_errno("could not open file " + path.string());
    }

    return fd;
}

} // namespace uh::cluster
