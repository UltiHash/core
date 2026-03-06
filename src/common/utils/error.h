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

#include <cstdint>
#include <exception>
#include <iosfwd>
#include <string>
#include <utility>

namespace uh::cluster {

class error {
public:
    enum type {
        success = 0,
        unknown = 1,
        bucket_not_found = 2,
        object_not_found = 3,
        bucket_not_empty = 4,
        invalid_bucket_name = 5,
        storage_limit_exceeded = 6,
        bucket_already_exists = 7,
        internal_network_error = 8,
        busy = 9,
        service_unavailable = 10,
    };

    error(type t = unknown, const std::string& message = "");
    error(uint32_t t, const std::string& message = "");

    const std::string& message() const;
    uint32_t code() const;
    type operator*() const;

private:
    type m_type;
    std::string m_message;
};

class error_exception : public std::exception {
public:
    error_exception(uh::cluster::error e = uh::cluster::error())
        : m_error(std::move(e)) {}

    const char* what() const noexcept override;

    const uh::cluster::error& error() const { return m_error; }

private:
    uh::cluster::error m_error;
};

std::string errno_message();
[[noreturn]] void throw_from_errno(std::string msg);

std::ostream& operator<<(std::ostream& out, const error& e);
std::ostream& operator<<(std::ostream& out, const error_exception& e);

} // namespace uh::cluster
