#ifndef COMMON_UTILS_HASH_H
#define COMMON_UTILS_HASH_H

#include <memory>
#include <openssl/evp.h>
#include <span>
#include <string>

namespace uh::cluster {

enum class hash_algorithm { md5, sha256 };

constexpr const char* SHA256_EMPTY_STRING =
    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

class hash_base {
public:
    hash_base(hash_algorithm algo);

    void reset();

    void consume(std::span<const char> data);

    std::string finalize();

private:
    hash_algorithm m_algo;
    std::unique_ptr<EVP_MD_CTX, void (*)(EVP_MD_CTX*)> m_ctx;
};

template <hash_algorithm algo> struct hash : public hash_base {
    hash()
        : hash_base(algo) {}

    /**
     * Compute checksum of provided string and return it as hexadecimal string.
     * @throws on error
     */
    static std::string from_buffer(std::span<const char> input) {
        hash h;

        h.consume(input);

        return h.finalize();
    }

    static std::string from_string(std::string_view s) {
        return from_buffer({s.begin(), s.size()});
    }
};

using md5 = hash<hash_algorithm::md5>;
using sha256 = hash<hash_algorithm::sha256>;

} // namespace uh::cluster

#endif
