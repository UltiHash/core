#ifndef COMPRESSION_NONE_H
#define COMPRESSION_NONE_H

#include <io/device.h>


namespace uh::comp
{

// ---------------------------------------------------------------------

class none : public io::device
{
public:
    none(io::device& base);

    std::streamsize write(std::span<const char> buffer) override;

    std::streamsize read(std::span<char> buffer) override;

    bool valid() const override;

private:
    io::device& m_base;
};

// ---------------------------------------------------------------------

} // namespace uh::comp

#endif
