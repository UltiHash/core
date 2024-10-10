#ifndef CORE_COMMON_CRYPTO_SCRYPT_H
#define CORE_COMMON_CRYPTO_SCRYPT_H

#include <memory>
#include <openssl/kdf.h>
#include <string>

namespace uh::cluster {

class scrypt {
public:
    struct config {
        uint64_t n = 1u << 17;
        uint32_t r = 8u;
        uint32_t p = 1u;

        uint32_t length = 32u;
    };

    scrypt(const config& c);

    std::string derive(std::string password, std::string salt);

private:
    config m_c;
};

} // namespace uh::cluster

#endif
