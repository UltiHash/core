#ifndef UTIL_TEMP_DIR_H
#define UTIL_TEMP_DIR_H

#include <filesystem>


namespace uh::util
{

// ---------------------------------------------------------------------

class temp_directory
{
public:
    temp_directory();
    ~temp_directory();

    const std::filesystem::path& path() const;

private:
    std::filesystem::path m_path;
};

// ---------------------------------------------------------------------

} // namespace uh::util

#endif
