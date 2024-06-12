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
 * Escapes given string for usage in XML documents.
 * @param str_to_encode a string to be escaped.
 * @return Returns a string which has been escaped for usage in an XML document.
 *
 */
std::string xml_escape(const std::string& str_to_encode);

/**
 * Return a string representing the provided char as hex string.
 */
template <typename T>
requires std::is_same_v <T, char> or std::is_same_v <T, unsigned char>
std::string to_hex(T value) {
    static constexpr auto hexChars = "0123456789abcdef";

    std::string result;
    result.push_back(hexChars[value >> 4]);
    result.push_back(hexChars[value & 0xf]);

    return result;
}

/**
 * Return a string representing the provided buffer as hex string.
 */
template <typename Array>
requires std::ranges::random_access_range <Array>
std::string to_hex(const Array& buffer) {
    std::string rv;

    for (auto n = 0ull; n < buffer.size(); ++n) {
        rv += to_hex(buffer[n]);
    }

    return rv;
}

} // namespace uh::cluster

#endif
