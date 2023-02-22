#include "f_serialization.h"

#include <utility>

namespace uh::client::serialization
{

// ---------------------------------------------------------------------------------------------------------------------

void serialize_stat(const struct stat_t& file_stat, std::vector<std::uint8_t>& bytes)
{

    uint16_t mode = file_stat.st_mode;
    std::copy_n(reinterpret_cast<const std::uint8_t*>(&mode), 2, std::back_inserter(bytes));

    uint64_t size = file_stat.st_size;
    std::copy_n(reinterpret_cast<const std::uint8_t*>(&size), 8, std::back_inserter(bytes));

    auto mtime = static_cast<std::uint64_t>(file_stat.st_mtime);
    std::copy_n(reinterpret_cast<const std::uint8_t*>(&mtime), 8, std::back_inserter(bytes));

    auto atime = static_cast<std::uint64_t>(file_stat.st_atime);
    std::copy_n(reinterpret_cast<const std::uint8_t*>(&atime), 8, std::back_inserter(bytes));
    auto ctime = static_cast<std::uint64_t>(file_stat.st_ctime);
    std::copy_n(reinterpret_cast<const std::uint8_t*>(&ctime), 8, std::back_inserter(bytes));

    auto uid = static_cast<uint32_t>(file_stat.st_uid);
    std::copy_n(reinterpret_cast<const std::uint8_t*>(&uid), 4, std::back_inserter(bytes));
    auto gid = static_cast<uint32_t>(file_stat.st_gid);
    std::copy_n(reinterpret_cast<const std::uint8_t*>(&gid), 4, std::back_inserter(bytes));

}

// ---------------------------------------------------------------------------------------------------------------------

void deserialize_stat(const struct stat_t& file_stat, std::vector<std::uint8_t>& bytes)
{

}

// ---------------------------------------------------------------------------------------------------------------------

f_serialization::f_serialization(std::filesystem::path UHV_path,
                                 common::job_queue<std::unique_ptr<common::f_meta_data>>& jq,const std::vector<std::filesystem::path>& root_paths) :
                                 m_UHV_path(std::move(UHV_path)), m_job_queue(jq), m_root_paths(root_paths)
{

}

// ---------------------------------------------------------------------------------------------------------------------

void f_serialization::serialize()
{
    std::fstream UHV_file(m_UHV_path, std::ios::app | std::ios::binary);

    if (!UHV_file.is_open())
    {
        throw std::runtime_error("Failed to open file " + m_UHV_path.string() + " when serializing.\n");
    }

    m_job_queue.stop();
    while (auto&& item = m_job_queue.get_job())
    {
        if (item == std::nullopt)
            break;

        std::vector<std::uint8_t> bytes;
        bytes.reserve(200);

        std::filesystem::path relative_path;
        for (const auto& root_path : m_root_paths)
        {
            relative_path = std::filesystem::relative(item.value()->get_f_path(), root_path);
        }
        EnDecoder coder{};
        SequentialContainer auto obj_name_vec=
                coder.encode<std::vector<std::uint8_t>>(relative_path.string());
        bytes.insert(bytes.cend(),obj_name_vec.begin(),obj_name_vec.end());

        serialize_stat(item.value()->get_f_stat(), bytes);

        EnDecoder coder2{};
        SequentialContainer auto obj_name_vec2 =
                coder2.encode<std::vector<std::uint8_t>>(item.value()->get_f_hashes());
        bytes.insert(bytes.cend(),obj_name_vec2.begin(),obj_name_vec2.end());

        std::for_each(bytes.cbegin(), bytes.cend(),
                      [&](const unsigned char character) { UHV_file << character; });
    }

    UHV_file.flush();
    UHV_file.close();
}

// ---------------------------------------------------------------------------------------------------------------------

void f_serialization::deserialize()
{
    std::fstream UHV_file(m_UHV_path, std::ios::in | std::ios::binary);

    if (!UHV_file.is_open())
    {
        throw std::runtime_error("Failed to open file " + m_UHV_path.string() + " when deserializing.\n");
    }

}

// ---------------------------------------------------------------------------------------------------------------------

} // namespace uh::client::serialization
