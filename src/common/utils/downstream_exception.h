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

#include <boost/system/system_error.hpp>
#include <exception>
#include <sstream>
#include <string>

namespace uh::cluster {

class downstream_exception : public std::exception {
public:
    downstream_exception(const std::string& msg,
                         const boost::system::system_error& original)
        : m_message(msg),
          m_original(original) {
        std::ostringstream oss;
        oss << "[downstream] " << msg << " | boost error: " << original.what();
        m_full_message = oss.str();
    }

    const char* what() const noexcept override {
        return m_full_message.c_str();
    }

    const boost::system::system_error& original_exception() const noexcept {
        return m_original;
    }

    const auto code() const { return m_original.code(); }

    const std::string& message() const noexcept { return m_message; }

private:
    std::string m_message;
    boost::system::system_error m_original;
    std::string m_full_message;
};

} // namespace uh::cluster
