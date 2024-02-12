#ifndef COMMON_UTILS_STRINGS_H
#define COMMON_UTILS_STRINGS_H

#include <ranges>
#include <string>
#include <vector>


namespace uh::cluster
{

/**
 * Split the provided string into a vector of `string_view`
 */
std::vector<std::string_view> split(std::string_view data, char delimiter = ' ');

}

#endif
