#pragma once
#include <openssl/evp.h>
#include <string>

namespace uh::cluster::rest::utils::hashing {

int hash_string(const char* string_to_hash);

class MD5 {
public:
    MD5();

    ~MD5();

    std::string calculateMD5(const std::string& input);

    static std::string calculateMD5_2(const std::string& input);

private:
    EVP_MD_CTX* pEvpContext;

    std::string toHex(unsigned char value);
};

} // namespace uh::cluster::rest::utils::hashing
