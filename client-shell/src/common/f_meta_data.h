#ifndef SERIALIZATION_FILE_META_DATA_H
#define SERIALIZATION_FILE_META_DATA_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <logging/logging_boost.h>


namespace uh::client::common
{

// ---------------------------------------------------------------------

enum uh_file_type : uint8_t
{
    none = 0,
    regular = 1,
    directory = 2,
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

class f_meta_data
{
public:

    f_meta_data() = default;
    explicit f_meta_data(std::filesystem::path);

    [[nodiscard]] const std::filesystem::path& f_path() const;
    [[nodiscard]] const std::vector<char>& f_hashes() const;
    [[nodiscard]] const std::uint8_t& f_type() const;
    [[nodiscard]] const std::uint32_t& f_permissions() const;
    [[nodiscard]] const std::uint64_t& f_size() const;

    void set_f_path(std::string);
    void set_f_type(const std::uint8_t&);
    void set_f_permissions(const std::uint32_t&);
    void set_f_size(const std::optional<std::uint64_t>&);
    void set_f_hashes(const std::string&);
    void add_hash(const std::vector<char>&);

private:
    std::filesystem::path m_f_path{};
    std::uint8_t m_f_type{};
    std::uint32_t m_f_permissions{};
    std::optional<std::uint64_t> m_f_size{};
    std::vector<char> m_f_hashes{};
};

// ---------------------------------------------------------------------

} // namespace uh::client::common

#endif