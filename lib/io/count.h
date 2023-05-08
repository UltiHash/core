#ifndef IO_COUNT_H
#define IO_COUNT_H

#include <io/device.h>


namespace uh::io
{

// ---------------------------------------------------------------------

class count : public device
{
public:
    count(device& base);

    std::streamsize write(std::span<const char> buffer) override;
    std::streamsize read(std::span<char> buffer) override;

    bool valid() const override;

    std::streamsize written() const;
    std::streamsize read() const;
private:
    device& m_base;
    std::streamsize m_written = 0;
    std::streamsize m_read = 0;
};

// ---------------------------------------------------------------------

} // namespace uh::io

#endif
