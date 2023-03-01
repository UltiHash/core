#include "f_meta_data.h"
#include <sys/stat.h>

namespace uh::client::common
{

// ---------------------------------------------------------------------------------------------------------------------

f_meta_data::f_meta_data(std::filesystem::path eval_path) :
    m_f_path(std::move(eval_path))
{

    std::filesystem::file_status f_status = std::filesystem::status(m_f_path);
    m_f_type = map_file_type<uh_file_type>(f_status.type());
    m_f_permissions = static_cast<uint32_t>(f_status.permissions());
    m_f_size = static_cast<std::uint64_t>(std::filesystem::file_size(m_f_path));

}

// ---------------------------------------------------------------------------------------------------------------------

void f_meta_data::add_hash(const std::vector<char>& hash)
{
    std::copy(hash.begin(), hash.end(), std::back_inserter(m_f_hashes));
}

// ---------------------------------------------------------------------------------------------------------------------

const std::filesystem::path& f_meta_data::get_f_path() const
{
    return m_f_path;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::vector<char>& f_meta_data::get_f_hashes() const
{
    return m_f_hashes;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::filesystem::file_type& f_meta_data::f_type() const
{
    return m_f_type;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::filesystem::perms& f_meta_data::permissions() const
{
    return m_f_permissions;
}

// ---------------------------------------------------------------------------------------------------------------------

void f_meta_data::set_f_path(std::string path_str)
{
    m_f_path = std::filesystem::path(std::move(path_str));
}

// ---------------------------------------------------------------------------------------------------------------------

void f_meta_data::set_f_hashes(const std::string& vec_hashes)
{
    m_f_hashes = std::vector<char>(vec_hashes.begin(), vec_hashes.end());
}

// ---------------------------------------------------------------------------------------------------------------------

} // namespace uh::client::common
