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
 * Convert give string to bool.
 */
bool to_bool(std::string str_to_eval);

/**
 * Return a string representing the provided char as hex string.
 */
std::string to_hex(unsigned char value);

/**
 * Return a string representing the provided buffer as hex string.
 */
std::string to_hex(std::span<char> buffer);

} // namespace uh::cluster

#endif
