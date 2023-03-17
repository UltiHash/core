#include <utility>
#include "f_serialization.h"

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

f_serialization::f_serialization(std::filesystem::path UHV_path,
                                 common::job_queue<std::unique_ptr<common::f_meta_data>>& jq) :
                                 m_UHV_path(std::move(UHV_path)), m_job_queue(jq)
{

}

// ---------------------------------------------------------------------

uint64_t f_serialization::serialize(const std::vector<std::filesystem::path>& root_paths)
{

    std::uint64_t raw_size = 0;
    std::uint64_t effective_size = 0;
    io::file f (m_UHV_path, std::ios::app | std::ios::binary);
    uh::serialization::buffered_serializer serialize (f);

    const auto count = m_job_queue.size();
    serialize.write(count);

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

        if (item.value()->f_type() == uh::client::common::uh_file_type::regular) {

            serialize.write(item.value()->f_size());
            serialize.write(item.value()->f_hashes());
            raw_size += item.value()->f_size();
            effective_size += item.value()->f_effective_size();

        }

    }
    std::cout << "de-duplication ratio: " << (double) effective_size / (double) raw_size << std::endl;

    return raw_size;
}

// ---------------------------------------------------------------------

uint64_t f_serialization::deserialize(const std::filesystem::path& dest_path)
{
    std::uint64_t raw_size = 0;

    io::file f (m_UHV_path);
    uh::serialization::sl_deserializer deserialize {f};
    const auto count = deserialize.read <unsigned long> ();

    for (auto i = 0; i < count; ++i) {

        auto p_f_meta_data = std::make_unique<uh::client::common::f_meta_data>();

        auto p = deserialize.read <std::string> ();
        p_f_meta_data->set_f_path(dest_path.string() + '/' + p);
        p_f_meta_data->set_f_type(deserialize.read <std::uint8_t> ());
        p_f_meta_data->set_f_permissions (deserialize.read <std::uint32_t> ());

        if (p_f_meta_data->f_type() == uh::client::common::uh_file_type::regular) {
            p_f_meta_data->set_f_size(deserialize.read <std::uint64_t> ());
            p_f_meta_data->set_f_hashes (deserialize.read <std::vector <char>> ());
        }

        // creating paths serially to avoid race condition - !!!
        if (p_f_meta_data->f_type() == uh::client::common::uh_file_type::regular) {
            std::ofstream(p_f_meta_data->f_path()).close();
            raw_size += p_f_meta_data->f_size();
        } else {
            std::filesystem::create_directory(p_f_meta_data->f_path());
        }

        m_job_queue.append_job(std::move(p_f_meta_data));
    }

    return raw_size;
}



// ---------------------------------------------------------------------

} // namespace uh::client::serialization
