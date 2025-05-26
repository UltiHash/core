#pragma once

#include <common/types/scoped_buffer.h>
#include <string>

namespace uh::cluster {

shared_buffer<char> random_buffer(
    std::size_t length = 16,
    const std::string& chars = "0123456789abcdefghijklmnopqrstuvwxyz");
}
