#include "meta_data.h"


namespace uh::uhv
{

// ---------------------------------------------------------------------

meta_data::meta_data(const std::filesystem::path& path)
    : m_path(path),
      m_size(std::nullopt)
{
    std::filesystem::file_status status = std::filesystem::status(m_path);

    m_type = convert_file_type<uh_file_type>(status.type());
    m_permissions = static_cast<uint32_t>(status.permissions());

    if (m_type == uh_file_type::regular)
    {
        m_size = std::filesystem::file_size(m_path);
    }
}

// ---------------------------------------------------------------------

void meta_data::add_hash(const std::vector<char>& hash)
{
    std::copy(hash.begin(), hash.end(), std::back_inserter(m_hashes));
}

// ---------------------------------------------------------------------

void meta_data::remove_hash(size_t start_index, size_t end_index)
{
    m_hashes.erase(m_hashes.begin() + start_index, m_hashes.begin() + end_index);
}

// ---------------------------------------------------------------------

void meta_data::add_effective_size(const std::uint64_t& size)
{
    m_effective_size += size;
}

// ---------------------------------------------------------------------

void meta_data::set_effective_size(const std::uint64_t& size)
{
    m_effective_size = size;
}

// ---------------------------------------------------------------------

const std::filesystem::path& meta_data::path() const
{
    return m_path;
}

// ---------------------------------------------------------------------

const std::vector<char>& meta_data::hashes() const
{
    return m_hashes;
}

// ---------------------------------------------------------------------

const std::vector<uint32_t>& meta_data::chunk_sizes () const
{
    return m_chunk_sizes;
}


// ---------------------------------------------------------------------

std::vector<char>& meta_data::get_hashes()
{
    return m_hashes;
}

// ---------------------------------------------------------------------

const std::uint8_t& meta_data::type() const
{
    return m_type;
}

// ---------------------------------------------------------------------

const std::uint32_t& meta_data::permissions() const
{
    return m_permissions;
}

// ---------------------------------------------------------------------

std::uint64_t meta_data::size() const
{
    return m_size.value_or(0u);
}

// ---------------------------------------------------------------------

const std::uint64_t& meta_data::effective_size() const
{
    return m_effective_size;
}

// ---------------------------------------------------------------------

void meta_data::set_path(std::string path_str)
{
    m_path = std::filesystem::path(std::move(path_str));
}

// ---------------------------------------------------------------------

void meta_data::set_hashes(const std::vector <char>& vec_hashes)
{
    m_hashes = vec_hashes;
}

// ---------------------------------------------------------------------

void meta_data::set_type(const std::uint8_t& type)
{
    m_type = type;
}

// ---------------------------------------------------------------------

void meta_data::set_permissions(const std::uint32_t& permissions)
{
    m_permissions = permissions;
}

// ---------------------------------------------------------------------

void meta_data::set_size(const std::optional<std::uint64_t>& size)
{
    m_size = size;
}

void meta_data::add_chunk_sizes(std::vector<uint32_t> &&chunk_sizes) {
    std::copy(chunk_sizes.begin(), chunk_sizes.end(), std::back_inserter(m_chunk_sizes));
}


void meta_data::add_chunk_sizes(const std::vector<uint32_t> &chunk_sizes) {
    std::copy(chunk_sizes.begin(), chunk_sizes.end(), std::back_inserter(m_chunk_sizes));
}
// ---------------------------------------------------------------------

} // namespace uh::uhv
