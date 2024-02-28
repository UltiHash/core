#pragma once
#include <openssl/evp.h>
#include <string>

namespace uh::cluster {

class md5 {
public:
    md5();
    ~md5();
    std::string calculate_md5(const std::string& input) const;

private:
    EVP_MD_CTX* m_ctx;
    static constexpr const char* EMPTY_MD5_HASH =
        "d41d8cd98f00b204e9800998ecf8427e";
};

} // namespace uh::cluster
