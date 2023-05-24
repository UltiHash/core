#include "meta_data.h"

namespace uh::uhv
{

// ---------------------------------------------------------------------

f_meta_data::f_meta_data(std::filesystem::path eval_path) :
    m_f_path(std::move(eval_path)), m_f_size(std::nullopt)
{

    std::filesystem::file_status f_status = std::filesystem::status(m_f_path);

    m_f_type = convert_file_type<uh_file_type>(f_status.type());
    m_f_permissions = static_cast<uint32_t>(f_status.permissions());

    if (m_f_type == uh_file_type::regular)
        m_f_size = static_cast<std::uint64_t>(std::filesystem::file_size(m_f_path));

}

// ---------------------------------------------------------------------

void f_meta_data::add_hash(const std::vector<char>& hash)
{
    std::copy(hash.begin(), hash.end(), std::back_inserter(m_f_hashes));
}

void f_meta_data::remove_hash (size_t start_index, size_t end_index) {
    m_f_hashes.erase(m_f_hashes.begin() + start_index, m_f_hashes.begin() + end_index);
}

// ---------------------------------------------------------------------

void f_meta_data::add_effective_size(const std::uint64_t& size)
{
    m_f_effective_size += size;
}

// ---------------------------------------------------------------------

void f_meta_data::set_effective_size(const std::uint64_t& size)
{
    m_f_effective_size = size;
}

// ---------------------------------------------------------------------

const std::filesystem::path& f_meta_data::f_path() const
{
    return m_f_path;
}

// ---------------------------------------------------------------------

const std::vector<char>& f_meta_data::f_hashes() const
{
    return m_f_hashes;
}

// ---------------------------------------------------------------------

const std::vector<uint32_t>& f_meta_data::f_chunk_sizes () const
{
    return m_chunk_sizes;
}


// ---------------------------------------------------------------------

std::vector<char>& f_meta_data::get_hashes()
{
    return m_f_hashes;
}

// ---------------------------------------------------------------------

const std::uint8_t& f_meta_data::f_type() const
{
    return m_f_type;
}

// ---------------------------------------------------------------------

const std::uint32_t& f_meta_data::f_permissions() const
{
    return m_f_permissions;
}

// ---------------------------------------------------------------------

const std::uint64_t& f_meta_data::f_size() const
{
    return m_f_size.value();
}

// ---------------------------------------------------------------------

const std::uint64_t& f_meta_data::f_effective_size() const
{
    return m_f_effective_size;
}

// ---------------------------------------------------------------------

void f_meta_data::set_f_path(std::string path_str)
{
    m_f_path = std::filesystem::path(std::move(path_str));
}

// ---------------------------------------------------------------------

void f_meta_data::set_f_hashes(const std::vector <char>& vec_hashes)
{
    m_f_hashes = vec_hashes;
}

// ---------------------------------------------------------------------

void f_meta_data::set_f_type(const std::uint8_t& f_type)
{
    m_f_type = f_type;
}

// ---------------------------------------------------------------------

void f_meta_data::set_f_permissions(const std::uint32_t& f_permissions)
{
    m_f_permissions = f_permissions;
}

// ---------------------------------------------------------------------

void f_meta_data::set_f_size(const std::optional<std::uint64_t>& f_size)
{
    m_f_size = f_size;
}

void f_meta_data::add_chunk_sizes(std::vector<uint32_t> &&chunk_sizes) {
    std::copy(chunk_sizes.begin(), chunk_sizes.end(), std::back_inserter(m_chunk_sizes));
}


void f_meta_data::add_chunk_sizes(const std::vector<uint32_t> &chunk_sizes) {
    std::copy(chunk_sizes.begin(), chunk_sizes.end(), std::back_inserter(m_chunk_sizes));
}
// ---------------------------------------------------------------------

} // namespace uh::uhv
