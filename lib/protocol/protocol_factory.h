//
// Created by masi on 14.03.23.
//

#ifndef CORE_PROTOCOL_FACTORY_H
#define CORE_PROTOCOL_FACTORY_H

#include <util/factory.h>
#include <memory>

#include "protocol.h"

namespace uh::protocol {

    class protocol_factory : public util::factory<uh::protocol::protocol> {
    public:

        virtual std::unique_ptr <protocol> create(std::shared_ptr <net::socket> socket) = 0;

    };
}
#endif //CORE_PROTOCOL_FACTORY_H
