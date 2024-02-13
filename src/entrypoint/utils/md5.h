#pragma once
#include <string>
#include <openssl/evp.h>

namespace uh::cluster
{

    struct md5
    {
        static std::string calculateMD5(const std::string& input);
        static std::string toHex(unsigned char value);

    };
} // uh::cluster::rest::utils::hashing
