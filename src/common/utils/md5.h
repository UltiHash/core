#ifndef COMMON_UTILS_MD5_H
#define COMMON_UTILS_MD5_H

#include <string>

namespace uh::cluster {

/**
 * Compute MD5 checksum of provided string and return it as hexadecimal string.
 * @throws on error
 */
std::string calculate_md5(const std::string& input);

} // namespace uh::cluster

#endif
