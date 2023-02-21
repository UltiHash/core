#include "f_serialization.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------------------------------------------------------

// Serializes a struct stat64 into a vector of bytes.
void serialize_stat(const struct stat_t& file_stat, std::vector<uint8_t>& bytes)
{

    uint16_t mode = file_stat.st_mode;
    std::copy_n(reinterpret_cast<const uint8_t*>(&mode), 2, std::back_inserter(bytes));

    uint64_t size = file_stat.st_size;
    std::copy_n(reinterpret_cast<const uint8_t*>(&size), 8, std::back_inserter(bytes));

    auto mtime = static_cast<std::uint64_t>(file_stat.st_mtime);
    std::copy_n(reinterpret_cast<const uint8_t*>(&mtime), 8, std::back_inserter(bytes));

    auto atime = static_cast<std::uint64_t>(file_stat.st_atime);
    std::copy_n(reinterpret_cast<const uint8_t*>(&atime), 8, std::back_inserter(bytes));
    auto ctime = static_cast<std::uint64_t>(file_stat.st_ctime);
    std::copy_n(reinterpret_cast<const uint8_t*>(&ctime), 8, std::back_inserter(bytes));

    uint32_t uid = static_cast<uint32_t>(file_stat.st_uid);
    std::copy_n(reinterpret_cast<const uint8_t*>(&uid), 4, std::back_inserter(bytes));
    uint32_t gid = static_cast<uint32_t>(file_stat.st_gid);
    std::copy_n(reinterpret_cast<const uint8_t*>(&gid), 4, std::back_inserter(bytes));

}

// ---------------------------------------------------------------------------------------------------------------------

f_serialization::f_serialization(const std::filesystem::path& UHV_path,
                                 common::job_queue<std::unique_ptr<common::f_meta_data>>& jq) :
                                 m_UHV_path(UHV_path), m_job_queue(jq)
{

}

// ---------------------------------------------------------------------------------------------------------------------

void f_serialization::serialize()
{
    std::fstream UHV_file(m_UHV_path, std::ios::app);

    if (!UHV_file.is_open())
    {
        throw std::runtime_error("Failed to open file " + m_UHV_path.string() + " when serializing.\n");
    }

    m_job_queue.stop();
    while (auto&& item = m_job_queue.get_job())
    {
        if (item == std::nullopt)
            break;

        std::vector<uint8_t> bytes;
        bytes.reserve(32);

        // object names


        // meta-data
        serialize_stat(item.value()->get_f_stat(), bytes);

        // hashes

    }

    UHV_file.flush();
    UHV_file.close();
}

// ---------------------------------------------------------------------------------------------------------------------

void f_serialization::deserialize()
{
    std::fstream UHV_file(m_UHV_path, std::ios::in);

    if (!UHV_file.is_open())
    {
        throw std::runtime_error("Failed to open file " + m_UHV_path.string() + " when deserializing.\n");
    }
}

// ---------------------------------------------------------------------------------------------------------------------

} // namespace uh::client::serialization
