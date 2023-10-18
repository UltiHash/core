//
// Created by masi on 10/17/23.
//

#ifndef CORE_EC_FACTORY_H
#define CORE_EC_FACTORY_H

#include <memory>
#include "ec_non.h"
#include "ec_xor.h"

#include <common/common.h>

namespace uh::cluster {

    struct ec_factory {
        static std::unique_ptr <ec> make_ec (ec_type type) {

        }
    };


} // namespace uh::cluster

#endif //CORE_EC_FACTORY_H
