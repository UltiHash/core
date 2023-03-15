#ifndef IO_BUFFER_H
#define IO_BUFFER_H

#include <io/device.h>
#include <vector>


namespace uh::io
{

// ---------------------------------------------------------------------

class buffer : public device
{
public:
    buffer(std::size_t size);

    virtual std::streamsize write(std::span<const char> buffer) override;

    virtual std::streamsize read(std::span<char> buffer) override;

    virtual bool valid() const override;

private:
    std::vector<char> m_buffer;
    std::size_t m_wptr;
    std::size_t m_rptr;
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
