#pragma once

#include "../http_response.h"
#include "../http_request.h"

namespace uh::cluster::rest::http::model
{

    class delete_object_response : public http_response
    {
    public:
        explicit delete_object_response(const http_request&);
        delete_object_response(const http_request&, http::response<http::string_body>);

        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object() override;

    private:
        bool m_deleteMarkerHasBeenSet = false;
        std::string m_deleteMarker;

        bool m_versionIdHasBeenSet = false;
        std::string m_versionId;

        bool m_requestChargedHasBeenSet = false;
        std::string m_requestCharged;

        bool m_requestIdHasBeenSet = false;
        std::string m_requestId;
    };

} // namespace uh::cluster::rest::http::model
