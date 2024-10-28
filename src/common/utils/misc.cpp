#include "misc.h"

#include <fstream>

namespace uh::cluster {

std::string read_file(const std::filesystem::path& p) {
    constexpr std::size_t read_size = 4096;
    std::ifstream in(p);

    if (!in) {
        throw std::runtime_error("file does not exist: " + p.string());
    }

    std::string rv;
    std::string buffer(read_size, 0);
    while (in.read(buffer.data(), read_size)) {
        rv.append(buffer, 0, in.gcount());
    }

    rv.append(buffer, 0, in.gcount());
    return rv;
}

} // namespace uh::cluster
