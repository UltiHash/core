#include "md5.h"
#include <openssl/err.h>
#include <stdexcept>

namespace uh::cluster {

static std::string to_hex(unsigned char value) {
    static const char hexChars[] = "0123456789abcdef";
    std::string result;
    result.push_back(hexChars[value >> 4]);
    result.push_back(hexChars[value & 0xf]);
    return result;
}

static void throw_from_error(const std::string& prefix) {
    char buffer[256];
    ERR_error_string_n(ERR_get_error(), buffer, sizeof(buffer));

    throw std::runtime_error(prefix + ": " + std::string(buffer));
}

md5::md5()
    : m_ctx(nullptr) {
    m_ctx = EVP_MD_CTX_create();

    if (!m_ctx) {
        throw_from_error("cannot create MD context");
    }
}

md5::~md5() {
    if (m_ctx)
        EVP_MD_CTX_destroy(m_ctx);
}

std::string md5::calculate_md5(const std::string& input) {
    if (input.empty())
        return EMPTY_MD5_HASH;

    unsigned char unMdValue[EVP_MAX_MD_SIZE];
    unsigned int uiMdLength;

    if (!EVP_DigestInit_ex(m_ctx, EVP_md5(), nullptr)) {
        throw_from_error("error on digest initialization");
    }

    if (!EVP_DigestUpdate(m_ctx, input.c_str(), input.length())) {
        throw_from_error("error on digest update");
    }

    if (!EVP_DigestFinal_ex(m_ctx, unMdValue, &uiMdLength)) {
        throw_from_error("error on digest finalization");
    }

    std::string hex_md5;
    hex_md5.reserve(uiMdLength * 2 + 1);

    for (unsigned int i = 0; i < uiMdLength; i++) {
        hex_md5 += to_hex(unMdValue[i]);
    }

    return hex_md5;
}

} // namespace uh::cluster
