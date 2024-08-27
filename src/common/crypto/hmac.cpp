#include "hmac.h"

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

auto load_key(const std::string& key) {
    auto* pkey = EVP_PKEY_new_mac_key(
        EVP_PKEY_HMAC, nullptr,
        reinterpret_cast<const unsigned char*>(key.c_str()), key.size());

    if (!pkey) {
        throw_from_error("cannot read public key");
    }

    return std::unique_ptr<EVP_PKEY, void (*)(EVP_PKEY*)>(pkey, EVP_PKEY_free);
}

} // namespace

hmac_base::hmac_base(hash_algorithm algo, const std::string& key)
    : m_ctx(EVP_MD_CTX_create(), EVP_MD_CTX_free),
      m_key(load_key(key)) {
    if (!EVP_DigestSignInit(m_ctx.get(), nullptr, evp_algorithm(algo), nullptr,
                            m_key.get())) {
        throw_from_error("error on digest initialization");
    }
}

void hmac_base::consume(std::span<const char> data) {
    if (!EVP_DigestSignUpdate(m_ctx.get(), data.data(), data.size())) {
        throw_from_error("error on digest update");
    }
}

std::string hmac_base::finalize() {
    std::string hmac_value;
    hmac_value.resize(EVP_MAX_MD_SIZE);
    std::size_t length = EVP_MAX_MD_SIZE;

    if (!EVP_DigestSignFinal(
            m_ctx.get(), reinterpret_cast<unsigned char*>(hmac_value.data()),
            &length)) {
        throw_from_error("error on hmac finalization");
    }

    hmac_value.resize(length);
    return hmac_value;
}

} // namespace uh::cluster
