#ifndef SERIALIZATION_FILE_META_DATA_H
#define SERIALIZATION_FILE_META_DATA_H

#include <iostream>
#include <vector>
#include <filesystem>
#include <sys/stat.h>
#include <logging/logging_boost.h>

#if defined(BSD)
#define stat_t stat
#else
#define stat_t stat64
#endif

namespace uh::client::common
{

// ---------------------------------------------------------------------

class f_meta_data
{
public:

    // ------------------------------------------------- CLASS FUNCTIONS
    f_meta_data() = default;
    explicit f_meta_data(const std::filesystem::path&);

    // ------------------------------------------------- SPECIAL FUNCTIONS
    // write serializer and deserializer since it is the meta_data that is being serialized at the end

    // ------------------------------------------------- SETTER FUNCTIONS
    void add_hash(const std::vector<char>&);

    // ------------------------------------------------- GETTER FUNCTIONS
    [[nodiscard]] const std::filesystem::path& get_f_path() const;
    [[nodiscard]] const struct stat_t& get_f_stat() const;
    [[nodiscard]] const std::filesystem::file_type& get_f_type() const;

private:
    std::filesystem::path m_f_path;
    std::filesystem::file_type m_f_type;
    struct stat_t m_f_stat{};
    std::unique_ptr<std::vector<std::vector<char>>> m_f_hashes;
};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif