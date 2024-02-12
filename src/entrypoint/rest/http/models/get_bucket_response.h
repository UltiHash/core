#pragma once

#include "entrypoint/rest/http/http_response.h"
#include "entrypoint/rest/http/http_request.h"

namespace uh::cluster::rest::http::model
{

    class get_bucket_response : public http_response
    {
    public:
        explicit get_bucket_response(const http_request&);
        get_bucket_response(const http_request&, http::response<http::string_body>);

        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object() override;
        void add_bucket(std::string bucket_to_add);
        [[nodiscard]] const std::string& get_bucket() const;

    private:

        bool m_bucketHasBeenSet;
        std::string m_bucket;

        bool m_creationDateHasBeenSet;
        std::string m_creationDate;

    };

} // namespace uh::cluster::rest::http::model
