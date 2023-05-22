//
// Created by benjamin-elias on 22.05.23.
//

#include <cstdint>

#ifndef CORE_FRAGMENT_SIZE_STRUCT_H
#define CORE_FRAGMENT_SIZE_STRUCT_H

namespace uh::serialization{

    // ---------------------------------------------------------------------

    struct fragment_serialize_size_format{
        uint32_t header_size;
        uint32_t content_size;
        uint8_t index_num;

        fragment_serialize_size_format(uint32_t header_len,uint32_t content_len,uint8_t index_num):
                header_size(header_len), content_size(content_len), index_num(index_num){}

    };

    // ---------------------------------------------------------------------

}

#endif //CORE_FRAGMENT_SIZE_STRUCT_H
