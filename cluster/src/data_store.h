//
// Created by masi on 7/17/23.
//

#ifndef CORE_DATA_STORE_H
#define CORE_DATA_STORE_H

#include "cluster_config.h"

namespace uh::cluster {

class data_store {

public:
    data_store (uint128_t max_size, const data_store_config& conf):
        m_max_size (std::move (max_size)),
        m_conf (conf) {

    }

    const uint128_t m_max_size;
    const std::reference_wrapper <const data_store_config> m_conf;
};
} // end namespace uh::cluster

#endif //CORE_DATA_STORE_H
