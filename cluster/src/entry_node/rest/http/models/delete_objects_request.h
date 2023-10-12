#pragma once

#include <entry_node/rest/http/http_request.h>

namespace uh::cluster::rest::http::model
{

    class delete_objects_request : public http_request
    {
    public:
        explicit delete_objects_request(const http::request_parser<http::empty_body>&);

        ~delete_objects_request() override = default;

        [[nodiscard]] inline http_request_type get_request_name() const override { return http_request_type::DELETE_OBJECTS; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;


    private:

    };

} // uh::cluster::rest::http::model
