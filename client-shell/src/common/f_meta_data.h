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

enum uh_file_type : uint8_t
{
    none = 0,
    regular = 1,
    directory = 2,
};

// ---------------------------------------------------------------------------------------------------------------------

const std::unordered_map<std::filesystem::file_type, uh_file_type> u_map_file_type =
{

    {std::filesystem::file_type::none, uh_file_type::none},
    {std::filesystem::file_type::regular, uh_file_type::regular},
    {std::filesystem::file_type::directory, uh_file_type::directory}

};

// ---------------------------------------------------------------------------------------------------------------------

template <typename T, typename Y>
T map_file_type(const Y& y)
{
    if (std::is_same<T, uh_file_type>::value)
    {
        auto iter = u_map_file_type.find(y);

        if (iter != u_map_file_type.end())
        {
            return iter->second;
        }
        else
        {
            return uh_file_type::none;
        }
    }
    else
    {
        auto iter = std::find_if(u_map_file_type.begin(), u_map_file_type.end(),
                    [&](const auto& pair)
                    {
                        return pair.second == y;
                    });
        if (iter != u_map_file_type.end())
        {
            return iter->first;
        }
        else
        {
            return std::filesystem::file_type::not_found;
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

class f_meta_data
{
public:

    f_meta_data() = default;
    explicit f_meta_data(std::filesystem::path);

    [[nodiscard]] const std::filesystem::path& get_f_path() const;
    [[nodiscard]] const std::vector<char>& get_f_hashes() const;
    [[nodiscard]] const std::filesystem::file_type& f_type() const;
    [[nodiscard]] const std::filesystem::perms& permissions() const;

    void set_f_path(std::string);
    void set_f_hashes(const std::string&);
    void add_hash(const std::vector<char>&);

private:
    std::filesystem::path m_f_path{};
    uint8_t m_f_type{};
    std::uint32_t m_f_permissions{};
    std::uint64_t m_f_size{};
    std::vector<char> m_f_hashes{};
};

// ---------------------------------------------------------------------------------------------------------------------

} // namespace uh::client::common

#endif