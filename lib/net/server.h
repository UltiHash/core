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

    virtual void stop() {};

    [[nodiscard]] virtual bool is_busy () const = 0;
};

// ---------------------------------------------------------------------

} // namespace uh::net

#endif
