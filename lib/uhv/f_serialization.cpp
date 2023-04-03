#include <utility>
#include "f_serialization.h"


namespace uh::uhv
{

// ---------------------------------------------------------------------

std::vector<char> f_serialization::serialize_f_meta_data(const std::unique_ptr<uh::uhv::f_meta_data>& ptr_f_meta_data,
                                    const std::filesystem::path& relative_path)
{
    uh::io::sstream_device dev;
    uh::serialization::buffered_serializer serializer (dev);

    serializer.write(relative_path.string());
    serializer.write(ptr_f_meta_data->f_type());
    serializer.write(ptr_f_meta_data->f_permissions());

    if (ptr_f_meta_data->f_type() == uhv::uh_file_type::regular) {
        serializer.write(ptr_f_meta_data->f_size());
        serializer.write(ptr_f_meta_data->f_hashes());
    }

    return read_to_buffer(dev);
}

// ---------------------------------------------------------------------

/*
 * std::unique_ptr<uh::uhv::f_meta_data> f_serialization::deserialize_f_meta_data(std::vector<std::uint8_t>& uhv_container,
                                                                         std::vector<std::uint8_t>::iterator& it,
                                                                         const std::filesystem::path& dest_path)
{

    std::unique_ptr<uh::uhv::f_meta_data> p_f_meta_data = std::make_unique<uh::uhv::f_meta_data>();
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


    if (p_f_meta_data->f_type() == uh::uhv::uh_file_type::regular)
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
 */


// ---------------------------------------------------------------------

f_serialization::f_serialization(std::filesystem::path UHV_path,
                                 uhv::job_queue<std::unique_ptr<uhv::f_meta_data>>& jq, bool overwrite) :
                                 m_UHV_path(std::move(UHV_path)), m_job_queue(jq), m_overwrite(overwrite)
{
}

// ---------------------------------------------------------------------

uint64_t f_serialization::serialize(const std::vector<std::filesystem::path>& root_paths)
{

    std::uint64_t raw_size = 0;
    std::uint64_t effective_size = 0;

    auto mode = std::ios::app | std::ios::binary;
    if (m_overwrite) {
        mode = std::ios::trunc | std::ios::binary;
    }

    io::file f (m_UHV_path, mode);
    uh::serialization::buffered_serializer serialize (f);


    const auto count = m_job_queue.size();
    serialize.write(count);
    serialize.sync();


    // stopping the queue to signal the thread not to wait
    // else the thread will be in a waiting state if the queue is empty
    m_job_queue.stop();
    while (const auto& item = m_job_queue.get_job()) {
        if (item == std::nullopt)
            break;

        std::filesystem::path relative_path;
        if (root_paths[0] != item.value()->f_path()) [[likely]] {
            relative_path = std::filesystem::relative(item.value()->f_path(), root_paths[0].parent_path());
        } else {
            relative_path = root_paths[0].filename();
        }


        serialize.write(relative_path.string());
        serialize.write(item.value()->f_type());
        serialize.write(item.value()->f_permissions());

        if (item.value()->f_type() == uhv::uh_file_type::regular) {

            serialize.write(item.value()->f_size());
            serialize.write(item.value()->f_hashes());
            raw_size += item.value()->f_size();
            effective_size += item.value()->f_effective_size();

        }

        serialize.sync();

    }
    std::cout << "de-duplication ratio: " << (double) effective_size / (double) raw_size << std::endl;

    return raw_size;
}

// ---------------------------------------------------------------------

uint64_t f_serialization::deserialize(const std::filesystem::path& dest_path, bool create_files)
{
    std::uint64_t raw_size = 0;

    io::file f (m_UHV_path);
    uh::serialization::sl_deserializer deserialize {f};
    const auto count = deserialize.read <unsigned long> ();

    for (auto i = 0; i < count; ++i) {

        auto p_f_meta_data = std::make_unique<uhv::f_meta_data>();

        auto p = deserialize.read <std::string> ();
        p_f_meta_data->set_f_path(dest_path.string() + '/' + p);
        p_f_meta_data->set_f_type(deserialize.read <std::uint8_t> ());
        p_f_meta_data->set_f_permissions (deserialize.read <std::uint32_t> ());

        if (p_f_meta_data->f_type() == uhv::uh_file_type::regular) {
            p_f_meta_data->set_f_size(deserialize.read <std::uint64_t> ());
            p_f_meta_data->set_f_hashes (deserialize.read <std::vector <char>> ());
        }

        if (create_files) {
            // creating paths serially to avoid race condition - !!!
            if (p_f_meta_data->f_type() == uhv::uh_file_type::regular)
            {
                std::ofstream(p_f_meta_data->f_path()).close();
                raw_size += p_f_meta_data->f_size();
            }
            else
            {
                std::filesystem::create_directory(p_f_meta_data->f_path());
            }
        }

        m_job_queue.append_job(std::move(p_f_meta_data));
    }

    return raw_size;
}



// ---------------------------------------------------------------------

} // namespace uh::uhv
