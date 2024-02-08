#include "hash.h"

namespace uh::cluster::entry
{

    int hash_string(const char* strToHash)
    {
        if (!strToHash)
            return 0;

        unsigned hash = 0;
        while (char charValue = *strToHash++)
        {
            hash = charValue + 31 * hash;
        }

        return hash;
    }

    MD5::MD5()
    {
        pEvpContext = EVP_MD_CTX_create();
        EVP_MD_CTX_init(pEvpContext);
        EVP_DigestInit_ex(pEvpContext, EVP_md5(), nullptr);
    }

    MD5::~MD5()
    {
        EVP_MD_CTX_destroy(pEvpContext);
    }

    std::string MD5::toHex(unsigned char value)
    {
        static const char hexChars[] = "0123456789abcdef";
        std::string result;
        result.push_back(hexChars[value >> 4]);
        result.push_back(hexChars[value & 0xf]);
        return result;
    }

    std::string MD5::calculateMD5(const std::string& input)
    {
        unsigned char unMdValue[EVP_MAX_MD_SIZE];
        unsigned int uiMdLength;

        // Calculate MD5 for given string
        EVP_DigestUpdate(pEvpContext, input.c_str(), input.length());

        // Save MD5 into temp variable
        EVP_DigestFinal_ex(pEvpContext, unMdValue, &uiMdLength);

        // Copy the digest from the temp variable into the return value
        std::string md5hex;
        md5hex.reserve(uiMdLength * 2 + 1);

        for (unsigned int i = 0; i < uiMdLength; i++)
        {
            md5hex += toHex(unMdValue[i]);
        }

        return md5hex;
    }

} // uh::cluster::rest::utils::hashing
