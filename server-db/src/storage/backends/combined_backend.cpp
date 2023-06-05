//
// Created by masi on 6/5/23.
//

#include "combined_backend.h"

namespace uh::dbn::storage {

void combined_backend::smart_worker::operator()(std::filesystem::path path, std::vector<char> sha) {

}

void combined_backend::start() {

}

std::unique_ptr<io::data_generator> combined_backend::read_block(const std::span<char> &hash) {
    return std::unique_ptr<io::data_generator>();
}

std::pair<std::size_t, std::vector<char>> combined_backend::write_block(const std::span<char> &data) {
    auto res = m_hierarchical_storage.write_block(data);
    m_worker.push(m_hierarchical_storage.get_hash_path({res.second.data(), res.second.size()}), res.second);
    return std::move (res);
}

size_t combined_backend::free_space() {
    return 0;
}

size_t combined_backend::used_space() {
    return 0;
}

size_t combined_backend::allocated_space() {
    return 0;
}

std::string combined_backend::backend_type() {
    return std::string (m_type);
}

std::unique_ptr<uh::protocol::allocation> combined_backend::allocate(std::size_t size) {
    return std::unique_ptr<uh::protocol::allocation>();
}

std::unique_ptr<uh::protocol::allocation> combined_backend::allocate_multi(std::size_t size) {
    return std::unique_ptr<uh::protocol::allocation>();
}

} // namespace uh::dbn::storage
