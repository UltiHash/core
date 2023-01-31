//
// Created by benjamin-elias on 12.07.22.
//

#ifndef CMAKE_BUILD_DEBUG_ULTIHASH_IDENTIFIER_H
#define CMAKE_BUILD_DEBUG_ULTIHASH_IDENTIFIER_H

#include "protocol/common.h"
#include <openssl/sha.h>

//The identifier is the checksum of a block together with its identifying reference to the database
class Identifier {
    std::string hash;
public:
    template<class OutType>
    OutType encode() {
        EnDecoder coder{};
        return coder.encode<OutType>(hash);
    }

    template<class InType>
    typename InType::iterator decode(InType &iss,typename InType::iterator step) {
        EnDecoder coder{};
        auto dec_tup=coder.decoder<std::string>(iss,step);
        hash=std::get<0>(dec_tup);
        return std::get<1>(dec_tup);
    }

    std::string get(){
        return hash;
    }

    Identifier()=default;

    // TODO: research on optimization for best way to convert vector of chars to strings
    explicit Identifier(const uh::protocol::blob& nm_hash) {
        hash.clear();
        hash = std::string(nm_hash.begin(), nm_hash.end());
    }

    explicit Identifier(std::string& input){
        //generate sha512 to hash with openSSL library
        hash.clear();
        unsigned char hash_buf[SHA512_DIGEST_LENGTH];
        SHA512(reinterpret_cast<const unsigned char *>(input.c_str()), input.size(), hash_buf);
        hash = std::string(hash_buf, hash_buf + sizeof hash_buf / sizeof hash_buf[0]);
    }
};


#endif //CMAKE_BUILD_DEBUG_ULTIHASH_IDENTIFIER_H
