#include <utility>
#include <arpa/inet.h>
#include <endian.h>
#include "f_serialization.h"

namespace
{

// ---------------------------------------------------------------------------------------------------------------------

void serialize_f_meta_data(const std::unique_ptr<uh::client::common::f_meta_data>& ptr_f_meta_data,
                           std::vector<std::uint8_t>& bytes)
{
    bytes.reserve(100);
    serialize_to_vector(ptr_f_meta_data->f_type(), bytes);

    // !! use endecoder to encode integers

    // Serialize the file permissions
    std::uint16_t permissions = htons(static_cast<std::uint16_t>(ptr_f_meta_data->permissions()));
    std::copy_n(reinterpret_cast<const std::uint8_t*>(&permissions), 2, std::back_inserter(bytes));

    // w/o seri
    std::uint16_t test;
    const auto*p = reinterpret_cast<uint8_t *>(&test);
    std::memcpy(dest, src, sizeof(test));

    // w/ ser
    s.write(test);
}

// ---------------------------------------------------------------------------------------------------------------------

void deserialize_f_meta_data(struct stat_t& file_stat, const std::vector<std::uint8_t>& bytes,
                        std::vector<std::uint8_t>::iterator& it)
{



}

// ---------------------------------------------------------------------------------------------------------------------

} // namespace

namespace uh::client::serialization
{

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

    // stopping the queue to signal the thread not to wait
    // else the thread will be in a waiting state if the queue is empty
    m_job_queue.stop();
    while (const auto& item = m_job_queue.get_job())
    {
        if (item == std::nullopt)
            break;

        std::vector<std::uint8_t> bytes;

        // !!! Problem when multiple input paths are given
        std::filesystem::path relative_path;
        if (root_paths[0] != item.value()->get_f_path()) [[likely]]
        {
            relative_path = std::filesystem::relative(item.value()->get_f_path(), root_paths[0].parent_path());
        }
        else
        {
            relative_path = root_paths[0].filename();
        }

        EnDecoder coder{};
        SequentialContainer auto obj_name_vec=
                coder.encode<std::vector<std::uint8_t>>(relative_path.string());
        bytes.insert(bytes.cend(),obj_name_vec.begin(),obj_name_vec.end());

        serialize_f_meta_data(item.value(), bytes);

        // only files have hash vector to be serialized
        if (S_ISREG(item.value()->get_f_stat().st_mode))
        {
            SequentialContainer auto obj_name_vec2 =
                    coder.encode<std::vector<std::uint8_t>>(item.value()->get_f_hashes());
            bytes.insert(bytes.cend(),obj_name_vec2.begin(),obj_name_vec2.end());
        }

        std::for_each(bytes.cbegin(), bytes.cend(),
                      [&](const unsigned char character) { UHV_file << character; });

    }

    UHV_file.flush();
    UHV_file.close();
}

// ---------------------------------------------------------------------------------------------------------------------

void f_serialization::deserialize()
{
    std::ifstream UHV_file(m_UHV_path, std::ios::binary);

    if (!UHV_file.is_open())
    {
        throw std::runtime_error("Failed to open file " + m_UHV_path.string() + " when deserializing.\n");
    }

    UHV_file.seekg(0, std::ios::end);
    std::streamsize f_size = UHV_file.tellg();
    UHV_file.seekg(0);
    std::vector<std::uint8_t> UHV_container(f_size);
    if (!UHV_file.read(reinterpret_cast<char*>(UHV_container.data()), f_size).good())
    {
        throw std::runtime_error("Error reading contents of UHV file.\n");
    }

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

        if (S_ISREG(f_stat.st_mode))
        {
            auto decoded_hashes = coder.decoder<std::string>(UHV_container, step);
            p_f_meta_data->set_f_hashes(std::get<0>(decoded_hashes));
            step = std::get<1>(decoded_hashes);
        }

        m_job_queue.append_job(std::move(p_f_meta_data));

    }
}

// ---------------------------------------------------------------------------------------------------------------------

} // namespace uh::client::serialization
