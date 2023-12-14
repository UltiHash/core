//
// Created by masi on 10/17/23.
//

#ifndef CORE_EC_FACTORY_H
#define CORE_EC_FACTORY_H

#include <memory>
#include "ec_non.h"
#include "ec_xor.h"

#include "common/utils/common.h"

namespace uh::cluster {

    struct ec_factory {
        static std::unique_ptr <ec> make_ec (ec_type type) {
            switch (type) {
                case NONE:
                    return std::make_unique <ec_non> ();
                case XOR:
                    return std::make_unique <ec_xor> ();
                default:
                    throw std::runtime_error ("Unknown EC type!");
            }
        }
    };


} // namespace uh::cluster

#endif //CORE_EC_FACTORY_H
