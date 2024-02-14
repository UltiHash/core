#pragma once

#include "entrypoint/rest/http/http_request.h"

namespace uh::cluster::rest::http::model {

class list_objectsv2_request : public http_request {
  public:
    explicit list_objectsv2_request(
        const http::request_parser<http::empty_body>&,
        std::unique_ptr<rest::http::URI>);

    ~list_objectsv2_request() override = default;

    [[nodiscard]] inline http_request_type get_request_name() const override {
        return http_request_type::LIST_OBJECTS_V2;
    }

    [[nodiscard]] std::map<std::string, std::string>
    get_request_specific_headers() const override;

    inline coro<void> read_body(boost::asio::ip::tcp::socket& stream,
                                boost::beast::flat_buffer& buffer) override {
        co_return;
    }

  private:
    list_objectsv2_request&
    operator=(const http::request_parser<http::empty_body>& recv_req);

    std::string m_delimiter;
    bool m_delimiterHasBeenSet = false;

    std::string m_encodingType;
    bool m_encodingTypeHasBeenSet = false;

    int m_maxKeys;
    bool m_maxKeysHasBeenSet = false;

    std::string m_prefix;
    bool m_prefixHasBeenSet = false;

    std::string m_continuationToken;
    bool m_continuationTokenHasBeenSet = false;

    std::string m_fetchOwner;
    bool m_fetchOwnerHasBeenSet = false;

    std::string m_startAfter;
    bool m_startAfterHasBeenSet = false;

    std::string m_requestPayer;
    bool m_requestPayerHasBeenSet = false;

    std::string m_expectedBucketOwner;
    bool m_expectedBucketOwnerHasBeenSet = false;

    std::vector<std::string> m_optionalObjectAttributes;
    bool m_optionalObjectAttributesHasBeenSet = false;
};

} // namespace uh::cluster::rest::http::model
