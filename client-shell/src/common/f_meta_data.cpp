#if defined(BSD)
#define stat_t stat
#include <ctime>
#define a_time(f_stat) std::timespec{f_stat.st_atim, 0}
#define m_time(f_stat) std::timespec{f_stat.st_mtim, 0}
#define c_time(f_stat) std::timespec{f_stat.st_ctim, 0}
#else
#define stat_t stat64
#define a_time(f_stat) f_stat.st_atim
#define m_time(f_stat) f_stat.st_mtim
#define c_time(f_stat) f_stat.st_ctim
#endif

#include "f_meta_data.h"
#include <sys/stat.h>


namespace uh::client::common
{

// ---------------------------------------------------------------------------------------------------------------------

f_meta_data::f_meta_data(std::filesystem::path eval_path) :
    m_f_path(std::move(eval_path))
{

    std::filesystem::file_status f_status = std::filesystem::status(m_f_path);
    m_f_type = f_status.type();
    m_f_permissions = f_status.permissions();
    m_f_size = std::filesystem::file_size(m_f_path);

    struct stat_t f_stat{};
    if(stat_t(m_f_path.c_str(), &f_stat))
    {
        DEBUG << "The file stat of \"" << m_f_path << "\" could not be read!" << std::endl;
    }

    m_f_atime = a_time(f_stat);
    m_f_mtime = m_time(f_stat);
    m_f_ctime = c_time(f_stat);
    uid = f_stat.st_uid;
    gid = f_stat.st_gid;

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
