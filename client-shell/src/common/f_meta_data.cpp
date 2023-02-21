#include "f_meta_data.h"

#include <utility>

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
    m_f_hashes.push_back(hash);
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

std::string f_meta_data::print_hashes() const
{
    std::stringstream ss;

    for (const auto& hash : m_f_hashes)
    {
        for (const auto& ch : hash)
        {
            ss << ch;
        }
        ss << '\n';
    }

    return ss.str();
}

// ---------------------------------------------------------------------------------------------------------------------

} // namespace uh::client::common

