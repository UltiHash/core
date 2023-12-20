#pragma once

#include "entrypoint/rest/http/http_response.h"
#include "entrypoint/rest/http/http_request.h"

namespace uh::cluster::rest::http::model
{

    class delete_bucket_response : public http_response
    {
    public:
        explicit delete_bucket_response(const http_request&);
        delete_bucket_response(const http_request&, http::response<http::string_body>);

        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object() override;

    private:

    };

} // namespace uh::cluster::rest::http::model
