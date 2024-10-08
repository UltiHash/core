#include "ossl_base.h"

#include <openssl/err.h>

#include <stdexcept>

namespace uh::cluster {

void throw_from_error(const std::string& prefix) {
    char buffer[256];
    ERR_error_string_n(ERR_get_error(), buffer, sizeof(buffer));

    throw std::runtime_error(prefix + ": " + std::string(buffer));
}

} // namespace uh::cluster
