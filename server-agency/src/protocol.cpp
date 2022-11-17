#include "protocol.h"


using namespace boost::asio;

namespace uh::an
{

// ---------------------------------------------------------------------

void uh_protocol::handle(std::shared_ptr<connection> client)
{
    ip::tcp::iostream io(std::move(client->socket()));

    std::string in1, in2;
    io << "give me in1 ...\n";
    io >> in1;

    io << "received in ... \n";
    io << "give me in2 ...\n";
    io >> in2;

    if (in1.size() > in2.size())
    {
        io << "in1 is longer\n";
    }
    else
    {
        io << "in1 is not longer\n";
    }

    io.socket().close();
}

// ---------------------------------------------------------------------

} // namespace uh::an
