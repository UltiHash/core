#ifndef CORE_NODE_INTERFACE_H
#define CORE_NODE_INTERFACE_H

namespace uh::cluster
{

struct service_interface {
    virtual void run () = 0;
    virtual void stop () = 0;

    virtual ~service_interface () = default;
};

} // end namespace uh::cluster

#endif //CORE_NODE_INTERFACE_H
