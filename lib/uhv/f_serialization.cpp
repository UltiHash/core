#include <utility>
#include "f_serialization.h"


namespace uh::uhv
{

// ---------------------------------------------------------------------

void f_serialization::__serialize_f_meta_data(const std::unique_ptr<uh::uhv::f_meta_data>& ptr_f_meta_data,
                                              const std::filesystem::path& relative_path,
                                              io::device& device)
{
    uh::serialization::buffered_serializer serializer(device);

    serializer.write(relative_path.string());
    serializer.write(ptr_f_meta_data->f_type());
    serializer.write(ptr_f_meta_data->f_permissions());

    if (ptr_f_meta_data->f_type() == uhv::uh_file_type::regular) {
        serializer.write(ptr_f_meta_data->f_size());
        serializer.write(ptr_f_meta_data->f_hashes());
    }

    serializer.sync();
}

// ---------------------------------------------------------------------

std::vector<char> f_serialization::serialize_f_meta_data(const std::unique_ptr<uh::uhv::f_meta_data>& ptr_f_meta_data,
                                                         const std::filesystem::path& relative_path)
{
    uh::io::sstream_device dev;
    __serialize_f_meta_data(ptr_f_meta_data, relative_path, dev);
    return read_to_buffer(dev);
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::uhv::f_meta_data> f_serialization::__deserialize_f_meta_data(const std::filesystem::path& dest_path,
                                                                                 io::device& device)
{
    uh::serialization::sl_deserializer deserializer(device);
    std::unique_ptr<uh::uhv::f_meta_data> p_f_meta_data = std::make_unique<uh::uhv::f_meta_data>();

    auto p = deserializer.read <std::string> ();
    p_f_meta_data->set_f_path(dest_path.string() + '/' + p);
    p_f_meta_data->set_f_type(deserializer.read <std::uint8_t> ());
    p_f_meta_data->set_f_permissions(deserializer.read <std::uint32_t> ());

    if (p_f_meta_data->f_type() == uhv::uh_file_type::regular) {
        p_f_meta_data->set_f_size(deserializer.read <std::uint64_t> ());
        p_f_meta_data->set_f_hashes(deserializer.read <std::vector <char>> ());
    }

    return p_f_meta_data; // RVO optimization
}


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

    io::file file (m_UHV_path, mode);
    uh::serialization::buffered_serializer serialize (file);


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

        if (item.value()->f_type() == uhv::uh_file_type::regular) {
            raw_size += item.value()->f_size();
            effective_size += item.value()->f_effective_size();
        }

        __serialize_f_meta_data(item.value(), relative_path, file);

    }
    std::cout << "de-duplication ratio: " << (double) effective_size / (double) raw_size << std::endl;

    return raw_size;
}

// ---------------------------------------------------------------------

uint64_t f_serialization::deserialize(const std::filesystem::path& dest_path, bool create_files)
{
    std::uint64_t raw_size = 0;
    io::file file (m_UHV_path);
    uh::serialization::sl_deserializer deserialize (file);
    auto count = deserialize.read <unsigned long> ();

    while (count--)
    {
        auto p_f_meta_data = __deserialize_f_meta_data(dest_path, file);

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
