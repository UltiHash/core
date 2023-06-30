#ifndef UHV_META_DATA_H
#define UHV_META_DATA_H

#include <vector>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <logging/logging_boost.h>


namespace uh::uhv
{

// ---------------------------------------------------------------------

enum uh_file_type : uint8_t
{
    none = 0,
    regular,
    directory,
};

// ---------------------------------------------------------------------

const std::unordered_map<std::filesystem::file_type, uh_file_type> u_map_file_type =
{

    {std::filesystem::file_type::none, uh_file_type::none},
    {std::filesystem::file_type::regular, uh_file_type::regular},
    {std::filesystem::file_type::directory, uh_file_type::directory}

};

// ---------------------------------------------------------------------

template <typename T, typename Z>
T convert_file_type(const Z& z)
{

    if (std::is_same<T, uh_file_type>::value)
    {
        auto iter = u_map_file_type.find(z);

        if (iter != u_map_file_type.end())
        {
            return iter->second;
        }
        else
        {
            return uh_file_type::none;
        }
    }

}

// ---------------------------------------------------------------------

class meta_data
{
public:

    meta_data() = default;
    explicit meta_data(const std::filesystem::path& path);

    [[nodiscard]] const std::filesystem::path& path() const;
    [[nodiscard]] const std::vector<char>& hashes() const;
    [[nodiscard]] const std::vector<uint32_t>& chunk_sizes() const;
    [[nodiscard]] const std::uint8_t& type() const;
    [[nodiscard]] const std::uint32_t& permissions() const;
    [[nodiscard]] std::uint64_t size() const;
    [[nodiscard]] const std::uint64_t& effective_size() const;

    [[nodiscard]] std::vector<char>& get_hashes();


    void set_path(std::string);
    void set_type(const std::uint8_t&);
    void set_permissions(const std::uint32_t&);
    void set_size(const std::optional<std::uint64_t>&);
    void set_hashes(const std::vector <char>&);
    void add_hash(const std::vector<char>&);
    void add_chunk_sizes(std::vector<uint32_t>&&);
    void add_chunk_sizes(const std::vector<uint32_t>&);
    void remove_hash (size_t start_index, size_t end_index);
    void add_effective_size(const std::uint64_t&);
    void set_effective_size(const std::uint64_t&);

    template <typename iterator>
    void append_hashes(iterator begin, iterator end)
    {
        m_hashes.insert(m_hashes.end(), begin, end);
    }

    template <typename iterator>
    void append_sizes(iterator begin, iterator end)
    {
        m_chunk_sizes.insert(m_chunk_sizes.end(), begin, end);
    }

private:
    std::filesystem::path m_path{};
    std::uint8_t m_type{};
    std::uint32_t m_permissions{};
    std::optional<std::uint64_t> m_size{};
    std::uint64_t m_effective_size{};
    std::vector <uint32_t> m_chunk_sizes{};
    std::vector<char> m_hashes{};
};

// ---------------------------------------------------------------------

} // namespace uh::uhv

#endif
