#pragma once

#include "entry_node/rest/http/http_response.h"
#include "entry_node/rest/http/http_request.h"

namespace uh::cluster::rest::http::model
{

    class get_object_attributes_response : public http_response
    {
    public:
        explicit get_object_attributes_response(const http_request&);
        get_object_attributes_response(const http_request&, http::response<http::string_body>);

        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object() override;
        void set_etag(std::string);

    private:

        bool m_deleteMarkerHasBeenSet = false;
        bool m_deleteMarker;

        bool m_lastModifiedHasBeenSet;
        std::string m_lastModified;

        bool m_versionIdHasBeenSet;
        std::string m_versionId;

        bool m_requestChargedHasBeenSet;
        std::string m_requestCharged;

        bool m_eTagHasBeenSet;
        std::string m_eTag;

        bool m_checksumHasBeenSet;
        std::string m_checksum;

        bool m_objectPartsHasBeenSet;
        std::string m_objectParts;

        bool m_storageClassHasBeenSet;
        std::string m_storageClass;

        bool m_objectSizeHasBeenSet;
        long long m_objectSize;

        bool m_requestIdHasBeenSet;
        std::string m_requestId;

    };

} // namespace uh::cluster::rest::http::model
