#pragma once

#include <common/utils/common.h>
#include <string>
#include <string_view>

namespace uh::cluster {

struct license {
    /// string identifying the customer
    std::string customer;

    /// maximum allowed data referenced by the directory
    std::size_t max_data_store_size = 0;
};

/**
 * Read and verify the license given by `license_code`.
 *
 * If valid, returns a `license` structure that contains all fields listed
 * in the license. Otherwise throws `std::runtime_error`.
 */
license check_license(std::string_view license_code);

} // namespace uh::cluster
