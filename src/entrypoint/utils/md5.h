#pragma once
#include <openssl/evp.h>
#include <string>

namespace uh::cluster {

struct md5 {
    static std::string calculateMD5(const std::string& input);
    static std::string toHex(unsigned char value);
};
} // namespace uh::cluster
