#include "md5.h"
#include <memory>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <stdexcept>

namespace uh::cluster {

namespace {

std::string to_hex(unsigned char value) {
    static const char hexChars[] = "0123456789abcdef";
    std::string result;
    result.push_back(hexChars[value >> 4]);
    result.push_back(hexChars[value & 0xf]);
    return result;
}

void throw_from_error(const std::string& prefix) {
    char buffer[256];
    ERR_error_string_n(ERR_get_error(), buffer, sizeof(buffer));

    throw std::runtime_error(prefix + ": " + std::string(buffer));
}

constexpr const char* EMPTY_MD5_HASH = "d41d8cd98f00b204e9800998ecf8427e";

} // namespace

std::string calculate_md5(const std::string& input) {
    if (input.empty()) [[unlikely]]
        return EMPTY_MD5_HASH;

    auto ctx = std::unique_ptr<EVP_MD_CTX, void (*)(EVP_MD_CTX*)>(
        EVP_MD_CTX_create(), EVP_MD_CTX_free);

    unsigned char unMdValue[EVP_MAX_MD_SIZE];
    unsigned int uiMdLength;

    if (!EVP_DigestInit_ex(ctx.get(), EVP_md5(), nullptr)) {
        throw_from_error("error on digest initialization");
    }

    if (!EVP_DigestUpdate(ctx.get(), input.c_str(), input.length())) {
        throw_from_error("error on digest update");
    }

    if (!EVP_DigestFinal_ex(ctx.get(), unMdValue, &uiMdLength)) {
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
