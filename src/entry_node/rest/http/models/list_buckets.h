#pragma once

#include <entry_node/rest/http/http_request.h>

namespace uh::cluster::rest::http::model
{

    class list_buckets : public http_request
    {
    public:
        explicit list_buckets(const http::request_parser<http::empty_body>&);

        ~list_buckets() override = default;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::LIST_BUCKETS; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;


    private:

    };

} // uh::cluster::rest::http::model
