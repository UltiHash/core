
#include "temp_file.h"

namespace uh::io
{

namespace
{
// ---------------------------------------------------------------------

std::string gen_random() {
    static const char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    std::string tmp_s;
    tmp_s.reserve(6);
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 rand_gen(seed);
    std::uniform_int_distribution<uint16_t> distribution(0,sizeof(alphanum));

    for (int i = 0; i < 6; ++i) {
        tmp_s += alphanum[distribution(rand_gen) % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

temp_file::temp_file(const std::filesystem::path &directory):
file(directory,"a"),m_remove(true)
{
    temp_file_constructor(directory);
}

// ---------------------------------------------------------------------

temp_file::temp_file(const std::filesystem::path &directory, const std::string& mode):
io::file(directory,mode)
{
    temp_file_constructor(directory);
}

// ---------------------------------------------------------------------

temp_file::temp_file(const std::filesystem::path &directory, std::ios_base::openmode mode):
file(directory, mode)
{
    temp_file_constructor(directory);
}

// ---------------------------------------------------------------------

void temp_file::temp_file_constructor(const std::filesystem::path &directory)
{
    if (!std::filesystem::exists(directory))
    {
        THROW(util::exception, "parent of temporary file does not exist");
    }

    if(!std::filesystem::is_directory(directory))
    {
        THROW(util::exception,"the claimed directory \""+directory.string()+"\" was no directory");
    }

    std::string temp_path = generate_valid_temp_path(directory);

    m_path = temp_path;
    open();
}

// ---------------------------------------------------------------------

temp_file::~temp_file()
{
    close();

    if (m_remove)
    {
        delete_file();
    }
}

// ---------------------------------------------------------------------

void temp_file::release_to(const std::filesystem::path& path)
{
    m_remove = false;

    if (m_path == path)
    {
        return;
    }

    if (std::filesystem::exists(path))
    {
        THROW(util::file_exists,"the file of release "+path.string()+" did already exist");
    }
    else
    {
        rename(path);
    }

}

// ---------------------------------------------------------------------

void temp_file::rename(const std::filesystem::path& path)
{
    close();
    ::rename(m_path.c_str(),path.c_str());
    m_fp = freopen64(path.c_str(),m_mode.c_str(),m_fp);
    m_path = path;
}

// ---------------------------------------------------------------------

    std::filesystem::path temp_file::generate_valid_temp_path(const std::filesystem::path& at_directory) {
    const std::string FILENAME_TEMPLATE = "tempfile-";//6 characters

    std::filesystem::path out_approach;

    do{
        out_approach = at_directory / (FILENAME_TEMPLATE + gen_random());
    }
    while(std::filesystem::exists(out_approach));

    return out_approach;
}

// ---------------------------------------------------------------------

void temp_file::reset_temp_file() {
    freopen64(m_path.c_str(),"w",m_fp);
    freopen64(m_path.c_str(),m_mode.c_str(),m_fp);
}

// ---------------------------------------------------------------------

} // namespace uh::io
