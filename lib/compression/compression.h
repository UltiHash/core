#ifndef COMPRESSION_COMPRESSION_H
#define COMPRESSION_COMPRESSION_H

#include <io/device.h>
#include <compression/type.h>
#include <compression/none.h>
#include <compression/brotli.h>
#include <util/exception.h>
#include <filesystem>


namespace uh::comp
{

// ---------------------------------------------------------------------

/**
 * Create a device that can be used to compress and decompress data.
 *
 * For decompression, the device reads compressed data from the underlying `base`
 * device and returns it uncompressed. For compression, the device is fed with
 * raw data and writes it to the underlying `base` as compressed data.
 */
std::unique_ptr<io::device> create(std::unique_ptr<io::device>&& base, type t);
std::unique_ptr<io::device> create(io::device& base, type t);

// ---------------------------------------------------------------------

} // namespace uh::comp

#endif
