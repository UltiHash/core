#include "hash.h"
#include "common/utils/strings.h"

#include <openssl/err.h>
#include <stdexcept>

namespace uh::cluster {

namespace {

void throw_from_error(const std::string& prefix) {
    char buffer[256];
    ERR_error_string_n(ERR_get_error(), buffer, sizeof(buffer));

    throw std::runtime_error(prefix + ": " + std::string(buffer));
}

const EVP_MD* evp_algorithm(hash_algorithm algo) {
    switch (algo) {
    case hash_algorithm::md5:
        return EVP_md5();
    case hash_algorithm::sha256:
        return EVP_sha256();
    }

    throw std::runtime_error("unsupported hash algorithm");
}

} // namespace

hash_base::hash_base(hash_algorithm algo)
    : m_ctx(EVP_MD_CTX_create(), EVP_MD_CTX_free) {
    if (!EVP_DigestInit_ex(m_ctx.get(), evp_algorithm(algo), nullptr)) {
        throw_from_error("error on digest initialization");
    }
}

void hash_base::consume(std::span<const char> data) {
    if (!EVP_DigestUpdate(m_ctx.get(), data.data(), data.size())) {
        throw_from_error("error on digest update");
    }
}

std::string hash_base::finalize() {
    unsigned char unMdValue[EVP_MAX_MD_SIZE];
    unsigned int uiMdLength;
    if (!EVP_DigestFinal_ex(m_ctx.get(), unMdValue, &uiMdLength)) {
        throw_from_error("error on digest finalization");
    }

    std::string hex_hash;
    hex_hash.reserve(uiMdLength * 2 + 1);

    for (unsigned int i = 0; i < uiMdLength; i++) {
        hex_hash += to_hex(unMdValue[i]);
    }

    return hex_hash;
}

} // namespace uh::cluster
