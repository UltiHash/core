#ifndef CORE_COMMON_CRYPTO_OSSL_BASE_H
#define CORE_COMMON_CRYPTO_OSSL_BASE_H

#include <string>

namespace uh::cluster {

[[noreturn]] void throw_from_error(const std::string& prefix);

} // namespace uh::cluster

#endif
