#include "strings.h"


namespace uh::cluster
{

std::vector<std::string_view> split(std::string_view data, char delimiter) {
    auto split = data
        | std::ranges::views::split(delimiter)
        | std::ranges::views::transform([](auto&& str) { return std::string_view(&*str.begin(), std::ranges::distance(str)); });

    return { split.begin(), split.end() };
}



}
