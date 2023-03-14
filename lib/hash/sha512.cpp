#include "sha512.h"

#include <util/exception.h>
#include <openssl/evp.h>
#include <openssl/sha.h>


using namespace uh::io;

namespace uh::hash
{

// ---------------------------------------------------------------------

struct sha512::impl
{
    impl(device& base, std::unique_ptr<EVP_MD_CTX, void(*)(EVP_MD_CTX*)> ctx);
    device& base;
    std::unique_ptr<EVP_MD_CTX, void(*)(EVP_MD_CTX*)> ctx;
};

// ---------------------------------------------------------------------

sha512::impl::impl(device& base, std::unique_ptr<EVP_MD_CTX, void(*)(EVP_MD_CTX*)> ctx)
    : base(base), ctx(std::move(ctx))
{
}

// ---------------------------------------------------------------------

sha512::~sha512() = default;

// ---------------------------------------------------------------------

sha512::sha512(device& base)
    : m_impl(std::make_unique<impl>(base, std::unique_ptr<EVP_MD_CTX, void(*)(EVP_MD_CTX*)>( EVP_MD_CTX_new(), EVP_MD_CTX_free ) ))
{
    if (EVP_DigestInit_ex(m_impl->ctx.get(), EVP_sha512(), nullptr) != 1)
    {
        THROW(util::exception, "EVP_DigestInit failed for EVP_sha3_512");
    }
}

// ---------------------------------------------------------------------

std::streamsize sha512::write(std::span<const char> buffer)
{
    if (EVP_DigestUpdate(m_impl->ctx.get(), buffer.data(), buffer.size()) != 1)
    {
        THROW(util::exception, "EVP_DigestUpdate failed");
    }

    return m_impl->base.write(buffer);
}

// ---------------------------------------------------------------------

std::streamsize sha512::read(std::span<char> buffer)
{
    return m_impl->base.read(buffer);
}

// ---------------------------------------------------------------------

bool sha512::valid() const
{
    return m_impl->base.valid();
}

// ---------------------------------------------------------------------

std::vector<char> sha512::finalize()
{
    std::vector<char> rv(SHA512_DIGEST_LENGTH, 0);

    if (EVP_DigestFinal_ex(m_impl->ctx.get(), reinterpret_cast<unsigned char*>(rv.data()), nullptr) != 1)
    {
        THROW(util::exception, "EVP_DigestFinal_ex failed");
    }

    return rv;
}

// ---------------------------------------------------------------------

std::vector<char> sha512_digest(std::span<const char> buffer)
{
    std::vector<char> rv(SHA512_DIGEST_LENGTH);

    SHA512(
        reinterpret_cast<const unsigned char*>(buffer.data()), buffer.size(),
        reinterpret_cast<unsigned char*>(rv.data()));

    return rv;
}

// ---------------------------------------------------------------------

} // namespace uh::hash
