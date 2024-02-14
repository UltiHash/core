#pragma once

#include "entrypoint/rest/http/http_request.h"
#include "entrypoint/rest/http/http_response.h"

namespace uh::cluster::rest::http::model {

class put_object_response : public http_response {
  public:
    explicit put_object_response(const http_request&);

    [[nodiscard]] const http::response<http::string_body>&
    get_response_specific_object() override;
    void set_size(std::size_t);
    void set_effective_size(std::size_t);
    void set_space_savings(double);
    void set_bandwidth(double);

  private:
    bool m_bandwidthHasBeenSet = false;
    double m_bandwidth;

    bool m_originalSizeHasBeenSet = false;
    std::size_t m_originalSize;

    bool m_effectiveSizeHasBeenSet = false;
    std::size_t m_effectiveSize;

    bool m_spaceSavingsHasBeenSet = false;
    double m_spaceSavings;

    bool m_expirationHasBeenSet = false;
    std::string m_expiration;

    bool m_checksumCRC32HasBeenSet = false;
    std::string m_checksumCRC32;

    bool m_checksumCRC32CHasBeenSet = false;
    std::string m_checksumCRC32C;

    bool m_checksumSHA1HasBeenSet = false;
    std::string m_checksumSHA1;

    bool m_checksumSHA256HasBeenSet = false;
    std::string m_checksumSHA256;

    bool m_serverSideEncryptionHasBeenSet = false;
    std::string m_serverSideEncryption;

    bool m_versionIdHasBeenSet = false;
    std::string m_versionId;

    bool m_sSECustomerAlgorithmHasBeenSet = false;
    std::string m_sSECustomerAlgorithm;

    bool m_sSECustomerKeyMD5HasBeenSet = false;
    std::string m_sSECustomerKeyMD5;

    bool m_sSEKMSKeyIdHasBeenSet = false;
    std::string m_sSEKMSKeyId;

    bool m_sSEKMSEncryptionContextHasBeenSet = false;
    std::string m_sSEKMSEncryptionContext;

    bool m_bucketKeyEnabledHasBeenSet = false;
    std::string m_bucketKeyEnabled;

    bool m_requestChargedHasBeenSet = false;
    std::string m_requestCharged;

    bool m_requestIdHasBeenSet = false;
    std::string m_requestId;
};

} // namespace uh::cluster::rest::http::model
