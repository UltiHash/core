#ifndef ENTRYPOINT_HTTP_URI_H
#define ENTRYPOINT_HTTP_URI_H

#include "boost/url/url.hpp"
#include <boost/beast/http.hpp>
#include <map>
#include <string>

namespace uh::cluster {

namespace http = boost::beast::http; // from <boost/beast/http.hpp>
typedef http::verb method;
typedef boost::urls::url url;

/**
 * Enum representing URI scheme.
 */
enum class scheme { HTTP, HTTPS };

class uri {
public:
    explicit uri(const http::request_parser<http::empty_body>::value_type&);

    /**
     * Access query string fields. Throws if name is not set.
     */
    const std::string& get(const std::string& name) const;

    /**
     * Access query string fields. Return std::nullopt if name is not set.
     */
    std::optional<std::string> get_opt(const std::string& name) const;

    /**
     * Return true if there are no parameters defined.
     */
    bool empty() const;

    /**
     * Return true if a given query parameter exists.
     */
    bool has(const std::string& name) const;

    const boost::urls::url& url() const { return m_url; }

private:
    void extract_query_parameters();

    boost::urls::url m_url;
    std::map<std::string, std::string> m_params;
};

} // namespace uh::cluster

#endif
