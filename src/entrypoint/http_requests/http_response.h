#ifndef ENTRYPOINT_HTTP_HTTP_RESPONSE_H
#define ENTRYPOINT_HTTP_HTTP_RESPONSE_H

#include "entrypoint/utils/md5.h"
#include "boost/beast.hpp"

namespace uh::cluster {

    namespace http = boost::beast::http;
    class http_response
    {
    public:
        http_response() = default;

        void set_body(std::string&& body);

        void set_effective_size(std::size_t effective_size);

        void set_space_savings(std::size_t space_savings);

        const http::response<http::string_body>& get_prepared_response();

        void set_bandwidth(double bandwidth);

    private:
        http::response<http::string_body> m_res {http::response<http::string_body>
                                                {http::status::ok, 11}};
        std::optional<std::string> m_etag;

        void set_etag(std::string etag);
    };

} // uh::cluster::entry

#endif
