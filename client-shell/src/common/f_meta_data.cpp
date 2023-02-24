#include "f_meta_data.h"

namespace uh::client::common
{

// ---------------------------------------------------------------------------------------------------------------------

f_meta_data::f_meta_data(std::filesystem::path eval_path) :
    m_f_path(std::move(eval_path))
{

    if(stat_t(m_f_path.c_str(), &m_f_stat))
    {
        DEBUG << "The file stat of \"" << m_f_path << "\" could not be read!" << std::endl;
    }

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

const struct stat_t& f_meta_data::get_f_stat() const
{
    return m_f_stat;
}

// ---------------------------------------------------------------------------------------------------------------------

const std::vector<char>& f_meta_data::get_f_hashes() const
{
    return m_f_hashes;
}

// ---------------------------------------------------------------------------------------------------------------------

void f_meta_data::set_f_path(const std::string& path_str)
{
    m_f_path = std::filesystem::path(path_str);
}

// ---------------------------------------------------------------------------------------------------------------------

void f_meta_data::set_f_hashes(const std::string& vec_hashes)
{
    m_f_hashes = std::vector<char>(vec_hashes.begin(), vec_hashes.end());
}

// ---------------------------------------------------------------------------------------------------------------------

void f_meta_data::set_f_stat_t(const struct stat_t& f_stat)
{
    m_f_stat = f_stat;
}

// ---------------------------------------------------------------------------------------------------------------------

} // namespace uh::client::common
