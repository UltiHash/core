#pragma once

#include "entry_node/rest/http/http_response.h"
#include "entry_node/rest/http/http_request.h"

namespace uh::cluster::rest::http::model
{

    class list_buckets_response : public http_response
    {
    public:
        explicit list_buckets_response(const http_request&);
        list_buckets_response(const http_request&, http::response<http::string_body>);

        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object() override;

        void add_bucket(std::string);

    private:

        bool m_bucketsHasBeenSet = false;
        std::vector<std::string> m_buckets {};

        bool m_ownerHasBeenSet = false;
        std::string m_owner {};

    };

} // namespace uh::cluster::rest::http::model
