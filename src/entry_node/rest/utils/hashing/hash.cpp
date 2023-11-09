#include "hash.h"

namespace uh::cluster::rest::utils::hashing
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

    std::string MD5::calculate(const std::string& str) const
    {

    }


} // uh::cluster::rest::utils::hashing