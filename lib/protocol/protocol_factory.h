//
// Created by masi on 14.03.23.
//

#ifndef CORE_PROTOCOL_FACTORY_H
#define CORE_PROTOCOL_FACTORY_H

#include <util/factory.h>
#include <memory>

#include "server.h"

namespace uh::protocol {

    class server_factory : public util::factory<uh::protocol::server> {
    public:

        virtual std::unique_ptr <protocol> create(std::shared_ptr <net::socket> socket)  {
            return std::make_unique<uh::protocol::server>(socket);
        }

    };
}
#endif //CORE_PROTOCOL_FACTORY_H
