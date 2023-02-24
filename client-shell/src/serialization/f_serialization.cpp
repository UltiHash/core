#include "f_serialization.h"

#include <utility>

namespace uh::client::serialization
{

// ---------------------------------------------------------------------------------------------------------------------

void serialize_stat_t(const struct stat_t& file_stat, std::vector<std::uint8_t>& bytes)
{

    uint16_t mode = file_stat.st_mode;
    std::copy_n(reinterpret_cast<const std::uint8_t*>(&mode), 2, std::back_inserter(bytes));

    uint64_t size = file_stat.st_size;
    std::copy_n(reinterpret_cast<const std::uint8_t*>(&size), 8, std::back_inserter(bytes));

    #ifdef BSD
        auto mtime = static_cast<std::uint64_t>(file_stat.st_mtime);
        auto atime = static_cast<std::uint64_t>(file_stat.st_atime);
        auto ctime = static_cast<std::uint64_t>(file_stat.st_ctime);
    #else
        auto mtime = static_cast<std::uint64_t>(file_stat.st_mtim.tv_sec);
        auto atime = static_cast<std::uint64_t>(file_stat.st_atim.tv_sec);
        auto ctime = static_cast<std::uint64_t>(file_stat.st_ctim.tv_sec);
    #endif

    std::copy_n(reinterpret_cast<const std::uint8_t*>(&mtime), 8, std::back_inserter(bytes));
    std::copy_n(reinterpret_cast<const std::uint8_t*>(&atime), 8, std::back_inserter(bytes));
    std::copy_n(reinterpret_cast<const std::uint8_t*>(&ctime), 8, std::back_inserter(bytes));

    auto uid = static_cast<uint32_t>(file_stat.st_uid);
    std::copy_n(reinterpret_cast<const std::uint8_t*>(&uid), 4, std::back_inserter(bytes));
    auto gid = static_cast<uint32_t>(file_stat.st_gid);
    std::copy_n(reinterpret_cast<const std::uint8_t*>(&gid), 4, std::back_inserter(bytes));

}

// ---------------------------------------------------------------------------------------------------------------------

void deserialize_stat_t(struct stat_t& file_stat, const std::vector<std::uint8_t>& bytes, std::vector<std::uint8_t>::iterator& it)
{

    // Deserialize st_mode (2 bytes)
    std::uint16_t mode;
    std::memcpy(&mode, &(*it), sizeof(mode));
    it += sizeof(mode);
    file_stat.st_mode = mode;

    // Deserialize st_size (8 bytes)
    std::uint64_t size;
    std::memcpy(&size, &(*it), sizeof(size));
    it += sizeof(size);
    file_stat.st_size = static_cast<decltype(file_stat.st_size)>(size);

    // Deserialize st_mtime (8 bytes)
    std::uint64_t mtime;
    std::memcpy(&mtime, &(*it), sizeof(mtime));
    it += sizeof(mtime);

    #if defined(BSD)
        // On BSD-based systems, st_mtime is a time_t value
        file_stat.st_mtime = static_cast<decltype(file_stat.st_mtime)>(mtime);
    #else
        // On other systems, st_mtime is a timespec structure
        std::timespec mtime_spec;
        mtime_spec.tv_sec = static_cast<decltype(mtime_spec.tv_sec)>(mtime);
        mtime_spec.tv_nsec = 0;
        file_stat.st_mtim = mtime_spec;
    #endif

    // Deserialize st_atime (8 bytes)
    std::uint64_t atime;
    std::memcpy(&atime, &(*it), sizeof(atime));
    it += sizeof(atime);

    #if defined(BSD)
        // On BSD-based systems, st_atime is a time_t value
        file_stat.st_atime = static_cast<decltype(file_stat.st_atime)>(atime);
    #else
        // On other systems, st_atime is a timespec structure
        std::timespec atime_spec;
        atime_spec.tv_sec = static_cast<decltype(atime_spec.tv_sec)>(atime);
        atime_spec.tv_nsec = 0;
        file_stat.st_atim = atime_spec;
    #endif

    // Deserialize st_ctime (8 bytes)
    std::uint64_t ctime;
    std::memcpy(&ctime, &(*it), sizeof(ctime));
    it += sizeof(ctime);

    #if defined(BSD)
        // On BSD-based systems, st_ctime is a time_t value
        file_stat.st_ctime = static_cast<decltype(file_stat.st_ctime)>(ctime);
    #else
        // On other systems, st_ctime is a timespec structure
        std::timespec ctime_spec;
        ctime_spec.tv_sec = static_cast<decltype(ctime_spec.tv_sec)>(ctime);
        ctime_spec.tv_nsec = 0;
        file_stat.st_ctim = ctime_spec;
    #endif

    // Deserialize st_uid (4 bytes)
    std::uint32_t uid;
    std::memcpy(&uid, &(*it), sizeof(uid));
    it += sizeof(uid);
    file_stat.st_uid = static_cast<decltype(file_stat.st_uid)>(uid);

    // Deserialize st_gid (4 bytes)
    std::uint32_t gid;
    std::memcpy(&gid, &(*it), sizeof(gid));
    it += sizeof(gid);
    file_stat.st_gid = static_cast<decltype(file_stat.st_gid)>(gid);

}

// ---------------------------------------------------------------------------------------------------------------------

f_serialization::f_serialization(std::filesystem::path UHV_path,
                                 common::job_queue<std::unique_ptr<common::f_meta_data>>& jq) :
                                 m_UHV_path(std::move(UHV_path)), m_job_queue(jq)
{

}

// ---------------------------------------------------------------------------------------------------------------------

void f_serialization::serialize(const std::vector<std::filesystem::path>& root_paths)
{
    std::ofstream UHV_file(m_UHV_path, std::ios::app | std::ios::binary);

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
        for (const auto& root_path : root_paths)
        {
            relative_path = std::filesystem::relative(item.value()->get_f_path(), root_path);
        }
        EnDecoder coder{};
        SequentialContainer auto obj_name_vec=
                coder.encode<std::vector<std::uint8_t>>(relative_path.string());
        bytes.insert(bytes.cend(),obj_name_vec.begin(),obj_name_vec.end());

        serialize_stat_t(item.value()->get_f_stat(), bytes);

        SequentialContainer auto obj_name_vec2 =
                coder.encode<std::vector<std::uint8_t>>(item.value()->get_f_hashes());
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
    std::ifstream UHV_file(m_UHV_path, std::ios::in | std::ios::binary);

    if (!UHV_file.is_open())
    {
        throw std::runtime_error("Failed to open file " + m_UHV_path.string() + " when deserializing.\n");
    }

    std::vector<std::uint8_t> UHV_container;
    char reader;
    while(UHV_file.get(reader))
    {
        UHV_container.push_back(reader);
    }
    UHV_file.close();

    auto step= UHV_container.begin();

    while(step != UHV_container.end())
    {
        EnDecoder coder{};
        struct stat_t f_stat{};
        std::unique_ptr<common::f_meta_data> p_f_meta_data = std::make_unique<common::f_meta_data>();

        // object name
        auto decoded_f_path = coder.decoder<std::string>(UHV_container, step);
        step = std::get<1>(decoded_f_path);
        p_f_meta_data->set_f_path(std::get<0>(decoded_f_path));

        // struct_t meta data
        deserialize_stat_t(f_stat, UHV_container, step);
        p_f_meta_data->set_f_stat_t(f_stat);

        // hashes
        auto decoded_hashes = coder.decoder<std::string>(UHV_container, step);
        p_f_meta_data->set_f_hashes(std::get<0>(decoded_hashes));
        step = std::get<1>(decoded_hashes);

        m_job_queue.put_back_job(std::move(p_f_meta_data));

    }
    m_job_queue.print();
}

// ---------------------------------------------------------------------------------------------------------------------

} // namespace uh::client::serialization
