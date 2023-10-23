#pragma once

#include "entry_node/rest/http/http_response.h"
#include "entry_node/rest/http/http_request.h"

namespace uh::cluster::rest::http::model
{

    class list_objectsv2_response : public http_response
    {
    public:
        explicit list_objectsv2_response(const http_request&);
        list_objectsv2_response(const http_request&, http::response<http::string_body>);

        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object() override;
        void add_content(std::string);
        void add_name(std::string);

    private:

        bool m_isTruncatedHasBeenSet = false;
        bool m_isTruncated;

        bool m_contentsHasBeenSet = false;
        std::vector<std::string> m_contents;

        bool m_nameHasBeenSet = false;
        std::string m_name;

        bool m_prefixHasBeenSet = false;
        std::string m_prefix;

        bool m_delimiterHasBeenSet = false;
        std::string m_delimiter;

        bool m_maxKeysHasBeenSet = false;
        int m_maxKeys;

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

        bool m_startAfterHasBeenSet = false;
        std::string m_startAfter;

        bool m_requestChargedHasBeenSet = false;
        std::string m_requestCharged;

        bool m_requestIdHasBeenSet = false;
        std::string m_requestId;

    };

} // namespace uh::cluster::rest::http::model
