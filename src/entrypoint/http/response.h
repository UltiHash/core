#ifndef CORE_ENTRYPOINT_HTTP_RESPONSE_H
#define CORE_ENTRYPOINT_HTTP_RESPONSE_H

#include "body.h"
#include <boost/beast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <common/types/common_types.h>

namespace uh::cluster::ep::http {

namespace beast = boost::beast;

using status = beast::http::status;

class response {
public:
    response();
    response(http::status status);

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

private:
    beast::http::response<beast::http::empty_body> m_res;
    std::unique_ptr<http::body> m_body;
};

/**
 * Append property_tree as XML to the response.
 */
response& operator<<(response& res, const boost::property_tree::ptree& pt);

std::ostream& operator<<(std::ostream& out, const response& res);

coro<void> write(boost::asio::ip::tcp::socket& out, response&& res);

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

#endif
