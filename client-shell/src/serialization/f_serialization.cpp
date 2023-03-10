#include <utility>
#include "f_serialization.h"

namespace {

// ---------------------------------------------------------------------

std::vector<std::uint8_t> serialize_f_meta_data(const std::unique_ptr<uh::client::common::f_meta_data>& ptr_f_meta_data,
                                    const std::filesystem::path& relative_path)
{

    std::vector<std::uint8_t> bytes;
    bytes.reserve(400);

    uh::client::serialization::EnDecoder coder{};

    SequentialContainer auto f_path_vec=
            coder.encode<std::vector<std::uint8_t>>(relative_path.string());
    bytes.insert(bytes.cend(),f_path_vec.begin(),f_path_vec.end());


    bytes.push_back(ptr_f_meta_data->f_type());

    auto ptr_perm_byte = reinterpret_cast<const std::uint8_t*>(&(ptr_f_meta_data->f_permissions()));
    bytes.insert(bytes.end(), ptr_perm_byte, ptr_perm_byte+sizeof(ptr_f_meta_data->f_permissions()));


    if (ptr_f_meta_data->f_type() == uh::client::common::uh_file_type::regular)
    {

        auto ptr_size_byte = reinterpret_cast<const std::uint8_t*>(&(ptr_f_meta_data->f_size()));
        bytes.insert(bytes.end(), ptr_size_byte, ptr_size_byte+sizeof(ptr_f_meta_data->f_size()));

        SequentialContainer auto obj_name_vec2 =
                coder.encode<std::vector<std::uint8_t>>(ptr_f_meta_data->f_hashes());
        bytes.insert(bytes.cend(),obj_name_vec2.begin(),obj_name_vec2.end());

    }

    return bytes; // RVO optimization

}

// ---------------------------------------------------------------------

std::unique_ptr<uh::client::common::f_meta_data> deserialize_f_meta_data(std::vector<std::uint8_t>& uhv_container,
                                                                         std::vector<std::uint8_t>::iterator& it,
                                                                         const std::filesystem::path& dest_path)
{

    std::unique_ptr<uh::client::common::f_meta_data> p_f_meta_data = std::make_unique<uh::client::common::f_meta_data>();
    uh::client::serialization::EnDecoder coder{};

    auto f_path_tuple = coder.decoder<std::string>(uhv_container, it);
    p_f_meta_data->set_f_path( dest_path.string() + '/' + std::get<0>(f_path_tuple));
    it = std::get<1>(f_path_tuple);


    std::uint8_t decoded_f_type = *it;
    p_f_meta_data->set_f_type(*it);
    std::advance(it, 1);

    std::uint32_t decoded_f_permissions;
    std::memcpy(&decoded_f_permissions, &(*it), sizeof(std::uint32_t));
    p_f_meta_data->set_f_permissions(decoded_f_permissions);
    std::advance(it, 4);


    if (p_f_meta_data->f_type() == uh::client::common::uh_file_type::regular)
    {

        std::uint64_t decoded_f_size;
        std::memcpy(&decoded_f_size, &(*it), sizeof(std::uint64_t));
        p_f_meta_data->set_f_size(decoded_f_size);
        std::advance(it, 8);

        auto decoded_hashes = coder.decoder<std::string>(uhv_container, it);
        p_f_meta_data->set_f_hashes(std::get<0>(decoded_hashes));
        it = std::get<1>(decoded_hashes);

    }

    return p_f_meta_data; // RVO optimization

}

// ---------------------------------------------------------------------

}

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

f_serialization::f_serialization(std::filesystem::path UHV_path,
                                 common::job_queue<std::unique_ptr<common::f_meta_data>>& jq) :
                                 m_UHV_path(std::move(UHV_path)), m_job_queue(jq)
{

}

// ---------------------------------------------------------------------

void f_serialization::serialize(const std::vector<std::filesystem::path>& root_paths)
{

    std::uint64_t raw_size = 0;
    std::uint64_t effective_size = 0;
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

        // !!! Problem when multiple input paths are given
        std::filesystem::path relative_path;
        if (root_paths[0] != item.value()->f_path()) [[likely]]
        {
            relative_path = std::filesystem::relative(item.value()->f_path(), root_paths[0].parent_path());
        }
        else
        {
            relative_path = root_paths[0].filename();
        }
        if (item.value()->f_type() == uh::client::common::uh_file_type::regular)
        {
            raw_size += item.value()->f_size();
            effective_size += item.value()->f_effective_size();

        }
        auto bytes = serialize_f_meta_data(item.value(), relative_path);

        UHV_file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));

    }

    INFO << "De-duplication ratio: " << (double) effective_size / (double) raw_size;
    UHV_file.flush();
    UHV_file.close();

}

// ---------------------------------------------------------------------

void f_serialization::deserialize(const std::filesystem::path& dest_path)
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
        auto p_f_meta_data = deserialize_f_meta_data(UHV_container, step, dest_path);

        // creating paths serially to avoid race condition - !!!
        if (p_f_meta_data->f_type() == uh::client::common::uh_file_type::regular)
            std::ofstream(p_f_meta_data->f_path()).close();
        else
            std::filesystem::create_directory(p_f_meta_data->f_path());

        m_job_queue.append_job(std::move(p_f_meta_data));
    }

}

// ---------------------------------------------------------------------

} // namespace uh::client::serialization
