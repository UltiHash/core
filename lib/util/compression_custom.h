//
// Created by benjamin-elias on 18.01.23.
//

#include <vector>

#ifndef UHLIBCOMMON_COMPRESSION_CUSTOM_H
#define UHLIBCOMMON_COMPRESSION_CUSTOM_H

namespace uh::util{
    class compression_custom {

        std::vector<unsigned char> compress(const std::vector<unsigned char> &input){
            return compress(input.begin(),input.end());
        }

        std::vector<unsigned char> decompress(const std::vector<unsigned char> &input){
            return decompress(input.begin(),input.end());
        }

        std::vector<unsigned char> compress(auto beg, auto end){
            return std::vector<unsigned char>{beg,end};
        }

        std::vector<unsigned char> decompress(auto beg, auto end){
            return std::vector<unsigned char>{beg,end};
        }
    };
}


#endif //UHLIBCOMMON_COMPRESSION_CUSTOM_H
