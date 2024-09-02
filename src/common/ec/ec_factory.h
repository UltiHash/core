#ifndef EC_FACTORY_H
#define EC_FACTORY_H

#include "no_ec.h"
#include "reedsolomon_c.h"

namespace uh::cluster {

struct ec_factory {
    static std::unique_ptr<ec_interface> create(size_t data_nodes,
                                                size_t ec_nodes) {
        if (data_nodes == 0) {
            throw std::invalid_argument("Number of data nodes cannot be zero");
        }
        if (ec_nodes > 0) {
            return std::make_unique<reedsolomon_c>(data_nodes, ec_nodes);
        }
        return std::make_unique<no_ec>(data_nodes);
    }
};

} // end namespace uh::cluster

#endif // EC_FACTORY_H
