#pragma once
#include <string>

namespace uh::cluster::rest::utils::hashing
{

    int hash_string(const char* string_to_hash);

    class MD5
    {
        MD5() = default;
        ~MD5() = default;

        /**
        * Calculates an MD5 hash
        */
        std::string calculate(const std::string& str) const;
    };

} // uh::cluster::rest::utils::hashing