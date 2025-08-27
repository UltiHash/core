#pragma once

#include <common/types/address.h>
#include <inttypes.h>
#include <string>

namespace uh::cluster {

struct object {
    uint64_t id;
    std::string name;
    std::size_t size{};

    address addr;
    std::string etag;
};

} // namespace uh::cluster
