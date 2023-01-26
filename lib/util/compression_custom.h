//
// Created by benjamin-elias on 18.01.23.
//

#include <vector>

#ifndef UHLIBCOMMON_COMPRESSION_CUSTOM_H
#define UHLIBCOMMON_COMPRESSION_CUSTOM_H

namespace uh::util{
    class compression_custom {
    public:

        template<class Container>
        std::vector<unsigned char> compress(const Container &input){
            return std::vector<unsigned char>{input.cbegin(),input.cend()};
        }

        template<class Container>
        std::vector<unsigned char> decompress(const Container &input){
            return std::vector<unsigned char>{input.cbegin(),input.cend()};
        }
    };
}


#endif //UHLIBCOMMON_COMPRESSION_CUSTOM_H
