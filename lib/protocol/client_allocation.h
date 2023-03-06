#ifndef PROTOCOL_CLIENT_ALLOCATION_H
#define PROTOCOL_CLIENT_ALLOCATION_H

#include "allocation.h"
#include "client.h"
#include "write_block_device.h"


namespace uh::protocol
{

// ---------------------------------------------------------------------

class client_allocation : public allocation
{
public:
    client_allocation(client& c);

    virtual io::device& device() override;
    virtual block_meta_data persist() override;

private:
    client& m_client;
    write_block_device m_device;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
