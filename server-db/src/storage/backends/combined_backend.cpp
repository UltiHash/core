//
// Created by masi on 6/5/23.
//

#include "combined_backend.h"

namespace uh::dbn::storage {

combined_backend::smart_worker::smart_worker(smart_storage &smart, storage_metrics &metrics) :
        m_smart_storage (smart), m_metrics (metrics) {}

void combined_backend::smart_worker::operator()(std::filesystem::path path, std::vector<char> sha) {
    int fd = open (path.c_str(), O_RDONLY);
    size_t file_size = lseek (fd, 0, SEEK_END);
    char* mmapped_data = static_cast <char*> (mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0));
    m_smart_storage.write_block({mmapped_data, file_size});
}

combined_backend::combined_backend(const hierarchical_storage_config &hierarchical_config,
                               state::scheduled_compressions_state& scheduled_compressions,
                               storage_metrics &storage_metrics):
    m_hierarchical_config(hierarchical_config),
    m_storage_metrics (storage_metrics),
    m_hierarchical_storage (hierarchical_config, m_storage_metrics, scheduled_compressions),
    m_smart_storage(hierarchical_config.smart_post_processing, m_storage_metrics),
    m_worker (hierarchical_config.smart_post_processing.number_of_threads, smart_worker (m_smart_storage, m_storage_metrics)) {

}

void combined_backend::start() {
    INFO << "--- Storage backend initialized --- " << std::filesystem::absolute(m_hierarchical_config.db_root);
    INFO << "        backend type   : " << backend_type();
    INFO << "        root directory : " << std::filesystem::absolute(m_hierarchical_config.db_root);
    INFO << "        space allocated: " << allocated_space();
    INFO << "        space available: " << free_space();
    INFO << "        space consumed : " << used_space();
}

std::unique_ptr<io::data_generator> combined_backend::read_block(const std::span<char> &hash) {
    try {
        return m_smart_storage.read_block(hash);
    }
    catch (std::out_of_range&) {
        return m_hierarchical_storage.read_block(hash);
    }
}

std::pair<std::size_t, std::vector<char>> combined_backend::write_block(const std::span<char> &data) {
    auto res = m_hierarchical_storage.write_block(data);
    m_worker.push(m_hierarchical_storage.get_hash_path({res.second.data(), res.second.size()}), res.second);
    return std::move (res);
}

size_t combined_backend::free_space() {
    return m_smart_storage.free_space();
}

size_t combined_backend::used_space() {
    return m_smart_storage.used_space();
}

size_t combined_backend::allocated_space() {
    return m_smart_storage.allocated_space();
}

std::string combined_backend::backend_type() {
    return std::string (m_type);
}

} // namespace uh::dbn::storage
