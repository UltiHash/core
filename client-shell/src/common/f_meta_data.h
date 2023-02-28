#ifndef SERIALIZATION_FILE_META_DATA_H
#define SERIALIZATION_FILE_META_DATA_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <logging/logging_boost.h>


namespace uh::client::common
{

// ---------------------------------------------------------------------------------------------------------------------

class f_meta_data
{
public:

    f_meta_data() = default;
    explicit f_meta_data(std::filesystem::path);

    [[nodiscard]] const std::filesystem::path& get_f_path() const;
    [[nodiscard]] const std::vector<char>& get_f_hashes() const;
    [[nodiscard]] const std::filesystem::file_type& f_type() const;

    void set_f_path(std::string);
    void set_f_hashes(const std::string&);
    void add_hash(const std::vector<char>&);

private:
    std::filesystem::path m_f_path;
    std::filesystem::file_type m_f_type;
    std::filesystem::perms m_f_permissions;
    std::uintmax_t m_f_size;
    std::timespec m_f_atime;
    std::timespec m_f_mtime;
    std::timespec m_f_ctime;
    std::uint_least32_t uid;
    std::uint_least32_t gid;
    std::vector<char> m_f_hashes;
};

// ---------------------------------------------------------------------------------------------------------------------

} // namespace uh::client::common

#endif