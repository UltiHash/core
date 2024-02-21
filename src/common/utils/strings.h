#ifndef COMMON_UTILS_STRINGS_H
#define COMMON_UTILS_STRINGS_H

#include <ranges>
#include <string>
#include <vector>

namespace uh::cluster {

/**
 * Split the provided string into a vector of `string_view`
 */
std::vector<std::string_view> split(std::string_view data,
                                    char delimiter = ' ');

/**
 * Decode a base64 encoded string to a buffer.
 */
std::vector<char> base64_decode(std::string_view b64);

/**
 * URL encode the special characters.
 */
std::string url_encode(const std::string&) noexcept;

/**
 * Find the closest matching string in a vector of strings given a comparison
 * string.
 */
std::vector<std::string>::const_iterator
find_lexically_closest(const std::vector<std::string>& strings,
                       const std::string& compareTo);

} // namespace uh::cluster

#endif
