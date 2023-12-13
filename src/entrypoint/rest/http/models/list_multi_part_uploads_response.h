#pragma once

#include "../http_response.h"
#include "../http_request.h"

namespace uh::cluster::rest::http::model
{

    typedef struct
    {
        std::string object_name;
        std::string upload_id;
    } key_and_uploadid;

    class list_multi_part_uploads_response : public http_response
    {
    public:
        explicit list_multi_part_uploads_response(const http_request&);
        list_multi_part_uploads_response(const http_request&, http::response<http::string_body>);

        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object() override;

        void add_key_and_uploadid(const std::string&, const std::string&);

    private:

        void populate_response_headers();

        std::vector<key_and_uploadid> m_keys_and_uploadids;

        bool m_bucketHasBeenSet = false;
        std::string m_bucket;

        bool m_keyMarkerHasBeenSet = false;
        std::string m_keyMarker;

        bool m_uploadIdMarkerHasBeenSet = false;
        std::string m_uploadIdMarker;

        bool m_nextKeyMarkerHasBeenSet = false;
        std::string m_nextKeyMarker;

        bool m_prefixHasBeenSet = false;
        std::string m_prefix;

        bool m_delimiterHasBeenSet = false;
        std::string m_delimiter;

        bool m_nextUploadIdMarkerHasBeenSet = false;
        std::string m_nextUploadIdMarker;

        bool m_maxUploadsHasBeenSet = false;
        int m_maxUploads;

        bool m_isTruncatedHasBeenSet = false;
        bool m_isTruncated;

        bool m_uploadsHasBeenSet = false;
        std::vector<std::string> m_uploads;

        bool m_commonPrefixesHasBeenSet = false;
        std::vector<std::string> m_commonPrefixes;

        bool m_encodingTypeHasBeenSet = false;
        std::string m_encodingType;

        bool m_requestChargedHasBeenSet = false;
        std::string m_requestCharged;

        bool m_uploadIdsHasBeenSet = false;
        std::vector<std::string> m_uploadIds;

        bool m_expectedBucketOwnerHasBeenSet = false;
        std::string m_expectedBucketOwner;

    };

} // namespace uh::cluster::rest::http::model
