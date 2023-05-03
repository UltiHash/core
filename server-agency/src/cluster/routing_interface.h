//
// Created by masi on 4/19/23.
//

#ifndef CORE_ROUTING_INTERFACE_H
#define CORE_ROUTING_INTERFACE_H

namespace uh::an::cluster
{

struct routing_interface {

    [[nodiscard]] virtual protocol::client_pool &route_data (const std::span <const char> &data) const = 0;

    virtual ~routing_interface() = default;

};

}

#endif //CORE_ROUTING_INTERFACE_H
