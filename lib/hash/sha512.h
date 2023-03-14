#ifndef HASH_SHA512_H
#define HASH_SHA512_H

#include <io/device.h>

#include <memory>


namespace uh::hash
{

// ---------------------------------------------------------------------

class sha512 : public io::device
{
public:
    sha512(io::device& base);
    virtual ~sha512();

    virtual std::streamsize write(std::span<const char> buffer) override;
    virtual std::streamsize read(std::span<char> buffer) override;
    virtual bool valid() const override;

    /**
     * Finalize the computation of the hash and return it.
     */
    std::vector<char> finalize();

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

std::vector<char> sha512_digest(std::span<const char> buffer);

// ---------------------------------------------------------------------

} // namespace uh::hash

#endif
