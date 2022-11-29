#ifndef PROTOCOL_SERIALIZER_HPP
#define PROTOCOL_SERIALIZER_HPP

#include <istream>
#include <limits>
#include <ostream>
#include <vector>


namespace uh::protocol
{

// ---------------------------------------------------------------------

#ifdef PROTOCOL_MAXIMUM_CHUNK_SIZE

constexpr std::size_t MAX_STRING_LENGTH = PROTOCOL_MAXIMUM_CHUNK_SIZE;
constexpr std::size_t MAX_BLOB_LENGTH = PROTOCOL_MAXIMUM_CHUNK_SIZE;

#else

constexpr std::size_t MAX_STRING_LENGTH = std::numeric_limits<uint32_t>::max();
constexpr std::size_t MAX_BLOB_LENGTH = std::numeric_limits<uint32_t>::max();

#endif

// ---------------------------------------------------------------------

void write(std::ostream& out, const std::vector<char>& b);
void read(std::istream& in, std::vector<char>& b);

void write(std::ostream& out, const std::string& s);
void read(std::istream& in, std::string& s);

void write(std::ostream& out, uint8_t value);
void read(std::istream& in, uint8_t& value);

void write(std::ostream& out, uint32_t value);
void read(std::istream& in, uint32_t& value);

void write(std::ostream& out, int32_t value);
void read(std::istream& in, int32_t& value);

// ---------------------------------------------------------------------

} // namespace uh::protocol

#endif
