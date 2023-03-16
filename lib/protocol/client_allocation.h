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
    virtual ~client_allocation();

    virtual io::device& device() override;
    virtual block_meta_data persist() override;

private:
    client& m_client;
    write_block_device m_device;
    bool m_dangling;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
