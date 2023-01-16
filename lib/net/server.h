#ifndef UH_NET_SERVER_H
#define UH_NET_SERVER_H

namespace uh::net
{

// ---------------------------------------------------------------------

class server
{
public:
    virtual ~server() = default;

    virtual void run() = 0;
};

// ---------------------------------------------------------------------

} // namespace uh::net

#endif
