#include "temp_directory.h"

#include "common/utils/random.h"

namespace uh::cluster {

namespace {

// ---------------------------------------------------------------------

std::filesystem::path make_temp_dir(const std::filesystem::path& parent) {
    std::filesystem::path path;
    std::error_code ec;

    do {
        path = parent / (std::string("tmp.") + random_string(8));
        std::filesystem::create_directory(path, ec);
    } while (ec);

    return path;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

temp_directory::temp_directory()
    : m_path(make_temp_dir(std::filesystem::temp_directory_path())) {}

// ---------------------------------------------------------------------

temp_directory::~temp_directory() {
    try {
        std::filesystem::remove_all(m_path);
    } catch (...) {
    }
}

// ---------------------------------------------------------------------

const std::filesystem::path& temp_directory::path() const { return m_path; }

// ---------------------------------------------------------------------

} // namespace uh::cluster
