/*
 * Copyright 2026 UltiHash Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

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
     * Compute checksum of provided string.
     * @return string containing the checksum, as characters, ie. non-hexed.
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
