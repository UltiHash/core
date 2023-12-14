#pragma once

#include "../http_response.h"
#include "../http_request.h"

namespace uh::cluster::rest::http::model
{

    class list_objects_response : public http_response
    {
    public:
        explicit list_objects_response(const http_request&);
        list_objects_response(const http_request&, http::response<http::string_body>);

        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object() override;
        void add_content(std::string);
        void add_name(std::string);
    private:

        void populate_response_headers();

        bool m_isTruncatedHasBeenSet = false;
        std::string m_isTruncated = "false";

        bool m_markerHasBeenSet = false;
        std::string m_marker;

        bool m_contentsHasBeenSet = false;
        std::vector<std::string> m_contents;

        bool m_nameHasBeenSet = false;
        std::string m_name;

        bool m_prefixHasBeenSet = false;
        std::string m_prefix;

        bool m_delimiterHasBeenSet = false;
        std::string m_delimiter;

        bool m_maxKeysHasBeenSet = false;
        int m_maxKeys = 1000;

        bool m_commonPrefixesHasBeenSet = false;
        std::vector<std::string> m_commonPrefixes;

        bool m_encodingTypeHasBeenSet = false;
        std::string m_encodingType;

        bool m_keyCountHasBeenSet = false;
        int m_keyCount;

        bool m_continuationTokenHasBeenSet = false;
        std::string m_continuationToken;

        bool m_nextContinuationTokenHasBeenSet = false;
        std::string m_nextContinuationToken;

        bool m_requestChargedHasBeenSet = false;
        std::string m_requestCharged;

        bool m_requestIdHasBeenSet = false;
        std::string m_requestId;

    };

} // namespace uh::cluster::rest::http::model
