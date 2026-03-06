// Copyright 2026 UltiHash Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
    : m_c(c) {}

std::string scrypt::derive(std::string password, std::string salt) {
    auto ctx = make_context();

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

    if (EVP_KDF_derive(ctx.get(), reinterpret_cast<unsigned char*>(rv.data()),
                       m_c.length, params) <= 0) {
        throw_from_error("scrypt key derivation failed");
    }

    return rv;
}

} // namespace uh::cluster
