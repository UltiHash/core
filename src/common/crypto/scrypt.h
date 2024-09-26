#ifndef CORE_COMMON_CRYPTO_SCRYPT_H
#define CORE_COMMON_CRYPTO_SCRYPT_H

#include <memory>
#include <openssl/kdf.h>
#include <string>

namespace uh::cluster {

class scrypt {
public:
    struct config {
        uint64_t n;
        uint32_t r;
        uint32_t p;

        uint32_t length;
    };

    scrypt(const config& c);

    std::string derive(std::string password, std::string salt);

private:
    std::unique_ptr<EVP_KDF_CTX, void (*)(EVP_KDF_CTX*)> m_ctx;
    config m_c;
};

} // namespace uh::cluster

#endif
