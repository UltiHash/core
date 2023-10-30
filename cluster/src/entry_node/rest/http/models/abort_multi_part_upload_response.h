#pragma once

#include "entry_node/rest/http/http_response.h"
#include "entry_node/rest/http/http_request.h"

namespace uh::cluster::rest::http::model
{

    class abort_multi_part_upload_response : public http_response
    {
    public:
        explicit abort_multi_part_upload_response(const http_request&);
        abort_multi_part_upload_response(const http_request&, http::response<http::string_body>);

        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object() override;

    private:

        bool m_requestChargedHasBeenSet = false;
        std::string m_requestCharged;

        bool m_requestIdHasBeenSet = false;
        std::string m_requestId;

    };

} // namespace uh::cluster::rest::http::model
