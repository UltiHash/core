#pragma once

#include "entry_node/rest/http/http_response.h"
#include "entry_node/rest/http/http_request.h"

namespace uh::cluster::rest::http::model
{

    class init_multi_part_upload_response : public http_response
    {
    public:
        explicit init_multi_part_upload_response(const http_request&);
        init_multi_part_upload_response(const http_request&, http::response<http::string_body>);

        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object() override;

        void set_upload_id(const std::string& upload_id);

    private:

        bool m_locationHasBeenSet = false;
        std::string m_location;

        bool m_uploadIdHasBeenSet = false;
        std::string m_uploadId;

        bool m_requestIdHasBeenSet = false;
        std::string m_requestId;

    };

} // namespace uh::cluster::rest::http::model
