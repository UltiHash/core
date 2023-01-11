#ifndef PROTOCOL_READ_BLOCK_DEVICE_H
#define PROTOCOL_READ_BLOCK_DEVICE_H

#include <io/device.h>


namespace uh::protocol
{

// ---------------------------------------------------------------------

class client;

// ---------------------------------------------------------------------

class read_block_device : public io::device
{
public:
    read_block_device(client& c);
    virtual ~read_block_device();

    virtual std::streamsize write(std::span<const char> buffer) override;
    virtual std::streamsize read(std::span<char> buffer) override;
    virtual bool valid() const override;

private:
    client& m_client;
};

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
