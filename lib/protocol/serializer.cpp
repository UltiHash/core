#include "serializer.h"

#include "exception.h"

#include <arpa/inet.h>


namespace uh::protocol
{

// ---------------------------------------------------------------------

std::streamsize read(std::istream& in, char* buffer, std::size_t count)
{
    if (!in.read(buffer, count))
    {
        THROW(read_error, "error reading buffer");
    }

    return in.gcount();
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const std::vector<char>& b)
{
    if (b.size() > MAX_BLOB_LENGTH)
    {
        THROW(write_limit_exceeded, "maximum supported vector length exceeded");
    }

    write(out, static_cast<uint32_t>(b.size()));
    out.write(&b[0], b.size());
}

// ---------------------------------------------------------------------

void read(std::istream& in, std::vector<char>& b)
{
    uint32_t count;
    read(in, count);

    std::vector<char> tmp;
    tmp.resize(count);

    read(in, &tmp[0], count);

    std::swap(tmp, b);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, std::span<const char> b)
{
    if (b.size() > MAX_BLOB_LENGTH)
    {
        THROW(write_limit_exceeded, "maximum supported buffer length exceeded");
    }

    write(out, static_cast<uint32_t>(b.size()));
    out.write(&b[0], b.size());
}

// ---------------------------------------------------------------------

void read(std::istream& in, std::span<char>& b)
{
    uint32_t count;
    read(in, count);

    if (count > b.size())
    {
        THROW(read_error, "remote returned buffer too big");
    }

    auto bytes = read(in, &b[0], count);
    if (bytes < b.size())
    {
        b = b.subspan(0, bytes);
    }
}

// ---------------------------------------------------------------------

void write(std::ostream& out, const std::string& s)
{
    if (s.size() > MAX_STRING_LENGTH)
    {
        THROW(write_limit_exceeded, "maximum supported string length exceeded");
    }

    write(out, static_cast<uint32_t>(s.size()));
    out.write(s.data(), s.size());
}

// ---------------------------------------------------------------------

void read(std::istream& in, std::string& s)
{
    uint32_t count;
    read(in, count);

    std::string tmp;
    tmp.resize(count);

    read(in, &tmp[0], count);

    std::swap(s, tmp);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, uint8_t value)
{
    out.put(value);
}

// ---------------------------------------------------------------------

void read(std::istream& in, uint8_t& value)
{
    auto ch = in.get();
    if (ch < 0)
    {
        THROW(read_error, "error reading from stream");
    }

    value = ch & 0xff;
}

// ---------------------------------------------------------------------

void write(std::ostream& out, uint32_t value)
{
    uint32_t nbo = htonl(value);
    out.write(reinterpret_cast<const char*>(&nbo), sizeof(nbo));
}

// ---------------------------------------------------------------------

void read(std::istream& in, uint32_t& value)
{
    uint32_t nbo;
    read(in, reinterpret_cast<char*>(&nbo), sizeof(nbo));
    value = ntohl(nbo);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, int32_t value)
{
    int32_t nbo = htonl(value);
    out.write(reinterpret_cast<const char*>(&nbo), sizeof(nbo));
}

// ---------------------------------------------------------------------

void read(std::istream& in, int32_t& value)
{
    int32_t nbo;
    read(in, reinterpret_cast<char*>(&nbo), sizeof(nbo));
    value = ntohl(nbo);
}

// ---------------------------------------------------------------------

void write(std::ostream& out, uint64_t value)
{
    int64_t nbo = htonl(value);
    out.write(reinterpret_cast<const char*>(&nbo), sizeof(nbo));
}

// ---------------------------------------------------------------------

void read(std::istream& in, uint64_t& value)
{
    int64_t nbo;
    read(in, reinterpret_cast<char*>(&nbo), sizeof(nbo));
    value = ntohl(nbo);
}

// ---------------------------------------------------------------------

} // namespace uh::protocol
