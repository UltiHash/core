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
    : m_algo(algo),
      m_ctx(EVP_MD_CTX_create(), EVP_MD_CTX_free) {
    if (!EVP_DigestInit_ex(m_ctx.get(), evp_algorithm(algo), nullptr)) {
        throw_from_error("error on digest initialization");
    }
}

void hash_base::reset() {

    if (!EVP_MD_CTX_reset(m_ctx.get())) {
        throw_from_error("reset failed");
    }

    if (!EVP_DigestInit_ex(m_ctx.get(), evp_algorithm(m_algo), nullptr)) {
        throw_from_error("error on digest initialization");
    }
}

void hash_base::consume(std::span<const char> data) {
    if (!EVP_DigestUpdate(m_ctx.get(), data.data(), data.size())) {
        throw_from_error("error on digest update");
    }
}

std::string hash_base::finalize() {
    std::string hash_value;
    hash_value.resize(EVP_MAX_MD_SIZE);
    unsigned int length = EVP_MAX_MD_SIZE;

    if (!EVP_DigestFinal_ex(m_ctx.get(),
                            reinterpret_cast<unsigned char*>(hash_value.data()),
                            &length)) {
        throw_from_error("error on hash finalization");
    }

    hash_value.resize(length);
    return hash_value;
}

} // namespace uh::cluster
