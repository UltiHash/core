//
// Created by masi on 10/24/23.
//

#ifndef CORE_NODE_INTERFACE_H
#define CORE_NODE_INTERFACE_H
namespace uh::cluster
{
struct node_interface {
    virtual void run () = 0;
    virtual void stop () = 0;
    virtual ~node_interface () = default;
};
} // end namespace uh::cluster

#endif //CORE_NODE_INTERFACE_H
