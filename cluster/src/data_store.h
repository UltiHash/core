//
// Created by masi on 7/17/23.
//

#ifndef CORE_DATA_STORE_H
#define CORE_DATA_STORE_H

#include "cluster_config.h"
#include <span>
namespace uh::cluster {

struct big_span {
    uint128_t pointer;
    size_t size;
};

class data_store {

public:
    data_store (data_store_config conf):
        m_conf (std::move (conf)) {

    }

    uint128_t write (std::span <char> data);
    big_span read (uint128_t pointer, size_t size) const;

    const data_store_config m_conf;
};
} // end namespace uh::cluster

#endif //CORE_DATA_STORE_H
