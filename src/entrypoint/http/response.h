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

#include <entrypoint/http/body.h>
#include <entrypoint/http/stream.h>
#include <common/types/common_types.h>

#include <boost/beast.hpp>
#include <boost/property_tree/ptree.hpp>

namespace uh::cluster::ep::http {

namespace beast = boost::beast;

using status = beast::http::status;

class response {
public:
    response();
    response(http::status status);
    response(beast::http::response<beast::http::empty_body> res,
             std::unique_ptr<http::body> body);

    void set_body(std::unique_ptr<http::body>&& body) noexcept;

    void set_effective_size(std::size_t effective_size);
    void set_original_size(std::size_t original_size);

    beast::http::response<beast::http::empty_body>& base() { return m_res; }
    const beast::http::response<beast::http::empty_body>& base() const {
        return m_res;
    }

    /**
     * Set value for header `header` to `value` or remove header in case
     * `value == std::nullopt`.
     */
    void set(const std::string& header, std::optional<std::string> value);
    void set(const std::string& header, std::optional<std::size_t> value);

    std::optional<std::string> header(const std::string& name) const;

    http::body& body() { return *m_body; }
    auto result() const { return m_res.result(); }

private:
    beast::http::response<beast::http::empty_body> m_res;
    std::unique_ptr<http::body> m_body;
};

/**
 * Append property_tree as XML to the response.
 */
response& operator<<(response& res, const boost::property_tree::ptree& pt);

std::ostream& operator<<(std::ostream& out, const response& res);

coro<void> write(stream& s, response&& res, const std::string& id);

template <typename value_type>
void put(boost::property_tree::ptree& tree, const std::string& key,
         value_type value) {
    tree.put(key, std::move(value));
}

template <typename value_type>
void put(boost::property_tree::ptree& tree, const std::string& key,
         std::optional<value_type> value) {
    if (value) {
        tree.put(key, *value);
    }
}

} // namespace uh::cluster::ep::http
