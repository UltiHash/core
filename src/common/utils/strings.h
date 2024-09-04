#ifndef COMMON_UTILS_STRINGS_H
#define COMMON_UTILS_STRINGS_H

#include <cstring>
#include <ranges>
#include <set>
#include <string>
#include <vector>

namespace uh::cluster {

/**
 * Split the provided string into a vector of `string_view`
 */
template <typename container = std::vector<std::string_view>>
container split(std::string_view data, char delimiter = ' ') {
    auto split =
        data | std::ranges::views::split(delimiter) |
        std::ranges::views::transform([](auto&& str) {
            return std::string_view(&*str.begin(), std::ranges::distance(str));
        });

    return {split.begin(), split.end()};
}

template <typename container = std::vector<std::string>>
std::string join(const container& c, const std::string& delimiter) {
    std::string rv;

    for (auto it = c.begin(); it != c.end(); ++it) {
        if (it != c.begin()) {
            rv += delimiter;
        }
        rv += *it;
    }

    return rv;
}

/**
 * Remove all characters specified in `chars` from the begin and end of `in`.
 */
std::string_view trim(std::string_view in,
                      std::string_view chars = " \n\r\t\f\v");
std::string_view ltrim(std::string_view in,
                       std::string_view chars = " \n\r\t\f\v");
std::string_view rtrim(std::string_view in,
                       std::string_view chars = " \n\r\t\f\v");

/**
 * Decode a base64 encoded string to a buffer.
 */
std::vector<char> base64_decode(std::string_view b64);

/**
 * URL encode the special characters.
 */
std::string url_encode(const std::string&) noexcept;

/**
 * URI encode as defined by
 * https://docs.aws.amazon.com/AmazonS3/latest/API/sig-v4-header-based-auth.html
 */
std::string uri_encode(const std::string& str,
                       const std::string& also_encode = "") noexcept;

/**
 * Return lower case version of the string.
 */
std::string lowercase(std::string s);

/**
 * Convert give string to bool.
 */
bool to_bool(std::string str_to_eval);

/**
 * Return a string representing the provided char as hex string.
 */
template <typename T>
requires std::is_same_v<T, char> or std::is_same_v<T, unsigned char>
std::string to_hex(T value) {
    static constexpr auto hexChars = "0123456789abcdef";

    std::string result;
    result.push_back(hexChars[(value >> 4) & 0xf]);
    result.push_back(hexChars[value & 0xf]);

    return result;
}

template <typename T>
requires std::is_same_v<T, char> or std::is_same_v<T, unsigned char>
std::string to_hexu(T value) {
    static constexpr auto hexChars = "0123456789ABCDEF";

    std::string result;
    result.push_back(hexChars[(value >> 4) & 0xf]);
    result.push_back(hexChars[value & 0xf]);

    return result;
}

/**
 * Return a string representing the provided buffer as hex string.
 */
template <typename Array>
requires std::ranges::random_access_range<Array>
std::string to_hex(const Array& buffer) {
    std::string rv;

    for (auto n = 0ull; n < buffer.size(); ++n) {
        rv += to_hex(buffer[n]);
    }

    return rv;
}

inline std::string operator+(std::string fst, std::string_view snd) {
    fst.resize(fst.size() + snd.size());
    std::memcpy(fst.data() + fst.size(), snd.data(), snd.size());
    return fst;
}

std::string unhex(std::string in);

} // namespace uh::cluster

#endif
