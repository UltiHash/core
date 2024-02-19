#include "strings.h"

#include <boost/beast/core/detail/base64.hpp>


using namespace boost;

namespace uh::cluster
{

std::vector<std::string_view> split(std::string_view data, char delimiter) {
    auto split = data
        | std::ranges::views::split(delimiter)
        | std::ranges::views::transform([](auto&& str) { return std::string_view(&*str.begin(), std::ranges::distance(str)); });

    return { split.begin(), split.end() };
}

std::vector<char> base64_decode(std::string_view b64) {
    std::vector<char> rv(beast::detail::base64::decoded_size(b64.size()));

    auto sizes = beast::detail::base64::decode(&rv[0], b64.data(), b64.size());
    rv.resize(sizes.first);

    return rv;
}

}
