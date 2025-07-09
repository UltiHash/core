#pragma once

#include <cstring>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

namespace uh::cluster {

static constexpr const char* CHARS_CAPITALS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static constexpr const char* CHARS_LOWERCASE = "abcdefghijklmnopqrstuvwxyz";
static constexpr const char* CHARS_DIGITS = "0123456789";
static constexpr const char* CHARS_SPECIAL =
    "!@#$%^&*()-_=+[]{}'\\\"|,./<>?<>~`";

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

std::string join(std::ranges::input_range auto&& range,
                 const std::string& delimiter) {
    std::string rv;
    bool first = true;

    for (const auto& e : range) {
        if (!first) {
            rv += delimiter;
        }

        rv += e;
        first = false;
    }

    return rv;
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
std::vector<char> base64_encode(std::string_view plain);

/**
 * Remove a set of characters from input string
 */
std::string erase_all(std::string haystack, const std::string& characters);

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
 * Compare both strings ignoring character case.
 */
bool equals_nocase(std::string_view a, std::string_view b);

struct nocase_less {
    struct is_transparent {};

    bool operator()(std::string_view a, std::string_view b) const {
        if (a.size() != b.size()) {
            return a.size() < b.size();
        }

        for (auto i = 0ull; i < a.size(); ++i) {
            if (tolower(a[i]) != tolower(b[i])) {
                return tolower(a[i]) < tolower(b[i]);
            }
        }

        return false;
    }

    bool operator()(auto& a, auto& b) const {
        return operator()(std::string_view(a), std::string_view(b));
    }
};

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

std::size_t stoul(std::string_view s, std::size_t* pos = nullptr,
                  int base = 10);

std::size_t ctoul(const char& c, std::size_t* pos = nullptr, int base = 10);

template <typename T>
concept HasToString = requires(const T& obj) {
    { obj.to_string() } -> std::convertible_to<std::string>;
};

template <typename T>
concept HasCreate = requires(const std::string& str) {
    { T::create(str) } -> std::same_as<T>;
};

template <typename T>
requires HasToString<T>
std::ostream& operator<<(std::ostream& os, const T& value) {
    os << value.to_string();
    return os;
}

template <typename T>
requires HasCreate<T>
std::istream& operator>>(std::istream& is, T& value) {
    std::string input;
    is >> input;

    try {
        value = T::create(input);
    } catch (const std::exception& e) {
        is.setstate(std::ios::failbit);
    }
    return is;
}

template <typename T>
std::string serialize(const T& value)
requires requires(std::ostream& os, const T& v) {
    { os << v } -> std::same_as<std::ostream&>;
}
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

template <typename T>
T deserialize(auto&& str)
requires requires(std::istream& is, T& v) {
    { is >> v } -> std::same_as<std::istream&>;
}
{
    std::istringstream iss(std::forward<decltype(str)>(str));
    T value;
    iss >> value;
    return value;
}

template <typename T>
std::string serialize(const std::optional<T>& opt)
requires requires(std::ostream& os, const T& v) {
    { os << v } -> std::same_as<std::ostream&>;
}
{
    if (!opt) {
        return "nullopt";
    }
    return serialize(*opt);
}

template <typename T>
concept IsOptional = requires {
    typename T::value_type;
    requires std::same_as<T, std::optional<typename T::value_type>>;
};

template <typename T>
T deserialize(auto&& str)
requires IsOptional<T> &&
         requires(std::istream& is, typename T::value_type& v) {
             { is >> v } -> std::same_as<std::istream&>;
         }
{
    std::string input = std::forward<decltype(str)>(str);
    if (input == "nullopt" || input.size() == 0) {
        return std::nullopt;
    }
    using ValueType = typename T::value_type;
    return deserialize<ValueType>(input);
}

template <typename Enum>
std::string serialize(Enum value)
requires std::is_enum_v<Enum>
{
    return std::to_string(static_cast<size_t>(value));
}

template <typename Enum>
Enum deserialize(auto&& str)
requires std::is_enum_v<Enum>
{
    size_t value = 0;
    value = stoul(std::forward<decltype(str)>(str));
    return static_cast<Enum>(value);
}

template <typename T>
auto split_buffer(std::span<T> buffer, size_t chunk_size) {
    if (buffer.size() % chunk_size != 0) {
        throw std::invalid_argument(
            "Buffer size is not a multiple of chunk size");
    }
    std::vector<std::span<T>> result;
    for (size_t offset = 0; offset < buffer.size(); offset += chunk_size) {
        result.push_back(buffer.subspan(offset, chunk_size));
    }
    return result;
}

} // namespace uh::cluster
