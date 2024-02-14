#include "md5.h"

namespace uh::cluster {

std::string md5::toHex(unsigned char value) {
    static const char hexChars[] = "0123456789abcdef";
    std::string result;
    result.push_back(hexChars[value >> 4]);
    result.push_back(hexChars[value & 0xf]);
    return result;
}

std::string md5::calculateMD5(const std::string& input) {
    EVP_MD_CTX* pEvpContext;
    pEvpContext = EVP_MD_CTX_create();
    EVP_MD_CTX_init(pEvpContext);
    EVP_DigestInit_ex(pEvpContext, EVP_md5(), nullptr);

    unsigned char unMdValue[EVP_MAX_MD_SIZE];
    unsigned int uiMdLength;

    // Calculate MD5 for given string
    EVP_DigestUpdate(pEvpContext, input.c_str(), input.length());

    // Save MD5 into temp variable
    EVP_DigestFinal_ex(pEvpContext, unMdValue, &uiMdLength);

    // Copy the digest from the temp variable into the return value
    std::string md5hex;
    md5hex.reserve(uiMdLength * 2 + 1);

    for (unsigned int i = 0; i < uiMdLength; i++) {
        md5hex += toHex(unMdValue[i]);
    }

    EVP_MD_CTX_destroy(pEvpContext);

    return md5hex;
}

} // namespace uh::cluster
