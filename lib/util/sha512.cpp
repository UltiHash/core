#include "sha512.h"

namespace uh::util
{

// ---------------------------------------------------------------------

uh::protocol::blob sha512(const uh::protocol::blob& some_data)
{

    DEBUG << "Beginning hash func";

    unsigned char result[SHA512_DIGEST_LENGTH];
    SHA512((unsigned char *) some_data.data(), some_data.size(), result);
    DEBUG << "sha512 calculated";

    uh::protocol::blob hash_vec(SHA512_DIGEST_LENGTH);
    memcpy(&hash_vec[0], &result[0], SHA512_DIGEST_LENGTH);

    return hash_vec;
}

// ---------------------------------------------------------------------

} // namespace uh::util
