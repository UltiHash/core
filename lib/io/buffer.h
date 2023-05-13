#ifndef IO_BUFFER_H
#define IO_BUFFER_H

#include <io/device.h>

#include <vector>


namespace uh::io
{

// ---------------------------------------------------------------------

class buffer : public io::device
{
public:
    buffer(std::size_t size = 1024);

    std::streamsize write(std::span<const char> buffer) override;
    std::streamsize read(std::span<char> buffer) override;

    bool valid() const override;

    const std::vector<char> &get_buffer();
private:
    std::vector<char> m_buffer;
    std::size_t m_rptr;
    std::size_t m_wptr;
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
