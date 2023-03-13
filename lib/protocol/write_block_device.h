#ifndef PROTOCOL_WRITE_BLOCK_DEVICE_H
#define PROTOCOL_WRITE_BLOCK_DEVICE_H

#include <io/device.h>
#include "client.h"


namespace uh::protocol
{

// ---------------------------------------------------------------------

class write_block_device : public io::device
{
public:
    write_block_device(client& c);
    virtual ~write_block_device();

    virtual std::streamsize write(std::span<const char> buffer) override;
    virtual std::streamsize read(std::span<char> buffer) override;
    virtual bool valid() const override;

private:
    client& m_client;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
