#pragma once

#include "hash.h"

namespace uh::cluster {

class hmac_base {
public:
    hmac_base(hash_algorithm algo, const std::string& key);

    void consume(std::span<const char> data);

    std::string finalize();

private:
    std::unique_ptr<EVP_MD_CTX, void (*)(EVP_MD_CTX*)> m_ctx;
    std::unique_ptr<EVP_PKEY, void (*)(EVP_PKEY*)> m_key;
};

template <hash_algorithm algo> struct hmac : public hmac_base {
    hmac(const std::string& key)
        : hmac_base(algo, key) {}

    /**
     * Compute HMAC of provided string, return it non-hexed.
     * @throws on error
     */
    static std::string from_buffer(const std::string& key,
                                   std::span<const char> input) {
        hmac h(key);

        h.consume(input);

        return h.finalize();
    }

    static std::string from_string(const std::string& key, std::string_view s) {
        return from_buffer(key, {s.begin(), s.size()});
    }
};

using hmac_md5 = hmac<hash_algorithm::md5>;
using hmac_sha256 = hmac<hash_algorithm::sha256>;

} // namespace uh::cluster
