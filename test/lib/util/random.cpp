#include "random.h"

#include <common/utils/random.h>

namespace uh::cluster {

shared_buffer<char> random_buffer(std::size_t length,
                                  const std::string& chars) {
    std::string random_str = random_string(length, chars);
    shared_buffer<char> buffer(length);
    std::copy(random_str.begin(), random_str.end(), buffer.data());
    return buffer;
}

} // namespace uh::cluster
