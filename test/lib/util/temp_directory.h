#pragma once

#include <filesystem>

namespace uh::cluster {

// ---------------------------------------------------------------------

class temp_directory {
public:
    temp_directory();
    ~temp_directory();

    [[nodiscard]] const std::filesystem::path& path() const;

private:
    std::filesystem::path m_path;
};

// ---------------------------------------------------------------------

} // namespace uh::cluster
