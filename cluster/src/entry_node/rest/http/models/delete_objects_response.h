#pragma once

#include "entry_node/rest/http/http_response.h"
#include "entry_node/rest/http/http_request.h"

namespace uh::cluster::rest::http::model
{

    class delete_objects_response : public http_response
    {
    public:
        explicit delete_objects_response(const http_request&);
        delete_objects_response(const http_request&, http::response<http::string_body>);

        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object() override;

    private:
        std::vector<std::string> m_deleted;

        bool m_requestChargedHasBeenSet = false;
        std::string m_requestCharged;

        std::vector<std::string> m_errors;

        std::string m_requestId;
    };

} // namespace uh::cluster::rest::http::model
