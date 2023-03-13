#ifndef PROTOCOL_PROTOCOL_H
#define PROTOCOL_PROTOCOL_H

#include <net/socket.h>
#include <memory>


namespace uh::protocol
{

// ---------------------------------------------------------------------

class protocol
{
protected:
    std::shared_ptr<net::socket> client_;
public:
    explicit protocol (std::shared_ptr<net::socket> client): client_(std::move (client)) {}
    virtual ~protocol() = default;
    virtual void handle() = 0;
};

/*
    class uhprotcol: public protocol { => server
            protocol(std::shared_ptr<net::socket> client)
            sera
        void handle() override {

        }
    };


    template <PtotocllTypw>
    class server { => plain_server


        proc.handle ();
    };
    */

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
