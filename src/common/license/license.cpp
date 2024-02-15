#include "license.h"

#include "common/utils/common.h"
#include "common/utils/log.h"
#include "common/utils/strings.h"
#include "license-public-key.inc"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

namespace uh::cluster {

namespace {

void throw_from_error(const std::string& prefix) {
    char buffer[256];
    ERR_error_string_n(ERR_get_error(), buffer, sizeof(buffer));

    throw std::runtime_error(prefix + ": " + std::string(buffer));
}

auto make_md_ctx() {
    auto* ctx = EVP_MD_CTX_create();

    if (!ctx) {
        throw_from_error("cannot create MD context");
    }

    return std::unique_ptr<EVP_MD_CTX, void (*)(EVP_MD_CTX*)>(ctx,
                                                              EVP_MD_CTX_free);
}

auto load_key(const unsigned char* data, std::size_t size) {
    auto* bio = BIO_new_mem_buf(data, size);
    if (!bio) {
        throw_from_error("cannot create BIO");
    }

    auto* key = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);

    BIO_free(bio);

    if (!key) {
        throw_from_error("cannot read public key");
    }

    return std::unique_ptr<EVP_PKEY, void (*)(EVP_PKEY*)>(key, EVP_PKEY_free);
}

bool verify(std::span<const char> data, const std::vector<char>& signature) {
    auto ctx = make_md_ctx();

    auto key = load_key(UH_LICENSE_PUBLIC_KEY, UH_LICENSE_PUBLIC_KEY_len);
    auto key_ctx = EVP_PKEY_CTX_new_from_pkey(nullptr, key.get(), nullptr);
    if (key_ctx == nullptr) {
        throw_from_error("EVP_PKEY_CTX_new");
    }

    EVP_MD_CTX_set_pkey_ctx(ctx.get(), key_ctx);

    if (!EVP_DigestVerifyInit_ex(ctx.get(), NULL, nullptr, nullptr, nullptr,
                                 key.get(), NULL)) {
        throw_from_error("EVP_DigestVerifyInit_ex");
    }

    return EVP_DigestVerify(
               ctx.get(),
               reinterpret_cast<const unsigned char*>(signature.data()),
               signature.size(),
               reinterpret_cast<const unsigned char*>(data.data()),
               data.size()) != 0;
}

} // namespace

license parse_license(std::string_view data) {
    auto fields = split(data, ':');

    return license{.customer = std::string(fields[0]),
                   .max_data_store_size =
                       std::stoull(std::string(fields[1])) * GIGA_BYTE};
}

license check_license(std::string_view license_code) {
    auto colon = license_code.rfind(':');
    if (colon == std::string::npos) {
        throw std::runtime_error("format error in license");
    }

    auto data = license_code.substr(0, colon);
    auto sign_b64 = license_code.substr(colon + 1);

    auto signature = base64_decode(sign_b64);

    if (!verify(data, signature)) {
        throw std::runtime_error("signature of license could not be verified");
    }

    return parse_license(data);
}

} // namespace uh::cluster
