//
// Created by benjamin-elias on 18.01.23.
//

#include <vector>

#ifndef UHLIBCOMMON_COMPRESSION_CUSTOM_H
#define UHLIBCOMMON_COMPRESSION_CUSTOM_H

namespace uh::util{
    class compression_custom {
    public:

        std::vector<unsigned char> compress(std::vector<unsigned char>&input){
            return std::vector<unsigned char>{input.begin(),input.end()};
        }

        std::vector<unsigned char> decompress(std::vector<unsigned char>&input){
            return std::vector<unsigned char>{input.begin(),input.end()};
        }
    };
}


#endif //UHLIBCOMMON_COMPRESSION_CUSTOM_H
