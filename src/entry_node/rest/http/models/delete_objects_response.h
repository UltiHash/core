#pragma once

#include "entry_node/rest/http/http_response.h"
#include "entry_node/rest/http/http_request.h"
#include "custom_error_response_exception.h"

namespace uh::cluster::rest::http::model
{

    struct fail
    {
        std::string key;
        uint32_t code;
    };

    class delete_objects_response : public http_response
    {
    public:
        explicit delete_objects_response(const http_request&);
        delete_objects_response(const http_request&, http::response<http::string_body>);

        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object() override;

        void add_deleted_keys(const std::string& key);
        void add_failed_keys(fail&& f);

    private:
        std::vector<std::string> m_deleted;

        bool m_requestChargedHasBeenSet = false;
        std::string m_requestCharged;

        std::vector<fail> m_errors;

        std::string m_requestId;
    };

} // namespace uh::cluster::rest::http::model
