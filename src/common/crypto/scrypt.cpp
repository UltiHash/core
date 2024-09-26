#include "scrypt.h"

#include "ossl_base.h"
#include <openssl/core_names.h>
#include <openssl/params.h>

namespace uh::cluster {

namespace {

std::unique_ptr<EVP_KDF_CTX, void (*)(EVP_KDF_CTX*)> make_context() {
    auto kdf = std::unique_ptr<EVP_KDF, void (*)(EVP_KDF*)>(
        EVP_KDF_fetch(NULL, "SCRYPT", NULL), EVP_KDF_free);
    if (!kdf) {
        throw_from_error("could not fetch scrypt algorithm");
    }

    auto rv = std::unique_ptr<EVP_KDF_CTX, void (*)(EVP_KDF_CTX*)>(
        EVP_KDF_CTX_new(kdf.get()), EVP_KDF_CTX_free);
    if (!rv) {
        throw_from_error("could not create KDF context");
    }

    return rv;
}

} // namespace

scrypt::scrypt(const config& c)
    : m_ctx(make_context()),
      m_c(c) {}

std::string scrypt::derive(std::string password, std::string salt) {
    OSSL_PARAM params[6];
    params[0] = OSSL_PARAM_construct_octet_string(
        OSSL_KDF_PARAM_PASSWORD, password.data(), password.size());
    params[1] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT,
                                                  salt.data(), salt.size());

    params[2] = OSSL_PARAM_construct_uint64(OSSL_KDF_PARAM_SCRYPT_N, &m_c.n);
    params[3] = OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_SCRYPT_R, &m_c.r);
    params[4] = OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_SCRYPT_P, &m_c.p);

    params[5] = OSSL_PARAM_construct_end();

    std::string rv(m_c.length, 0);

    if (EVP_KDF_derive(m_ctx.get(), reinterpret_cast<unsigned char*>(rv.data()),
                       m_c.length, params) <= 0) {
        throw_from_error("scrypt key derivation failed");
    }

    EVP_KDF_CTX_reset(m_ctx.get());

    return rv;
}

} // namespace uh::cluster
