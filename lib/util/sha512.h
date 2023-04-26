#ifndef UTIL_SHA512_H
#define UTIL_SHA512_H

#include <openssl/sha.h>
#include "logging/logging_boost.h"
#include "protocol/common.h"

namespace uh::util
{

// ---------------------------------------------------------------------

uh::protocol::blob sha512(const uh::protocol::blob& some_data);

// ---------------------------------------------------------------------

} // namespace uh::util

#endif