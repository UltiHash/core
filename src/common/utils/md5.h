#ifndef COMMON_UTILS_MD5_H
#define COMMON_UTILS_MD5_H

#include <memory>
#include <openssl/evp.h>
#include <span>
#include <string>

namespace uh::cluster {

class md5 {
public:
    md5();

    void consume(std::span<const char> data);

    std::string finalize();

private:
    std::unique_ptr<EVP_MD_CTX, void (*)(EVP_MD_CTX*)> m_ctx;
};

/**
 * Compute MD5 checksum of provided string and return it as hexadecimal string.
 * @throws on error
 */
std::string calculate_md5(const std::string& input);

} // namespace uh::cluster

#endif
