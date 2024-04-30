#include "fragment_set_log.h"
#include <fstream>

namespace uh::cluster {
fragment_set_log::fragment_set_log(std::filesystem::path log_path)
    : m_log_path(std::move(log_path)),
      m_log_file(m_log_path, std::fstream::binary | std::fstream::in |
                                 std::fstream::out | std::fstream::app) {
    if (!m_log_file.is_open()) {
        throw std::runtime_error("Could not open the transaction log file");
    }
}

void fragment_set_log::append(const log_entry& entry) {

    std::array<char, m_entry_size> buf{};
    zpp::bits::out{buf, zpp::bits::size4b{}}(entry).or_throw();
    std::lock_guard<std::mutex> guard(m_mutex);
    m_log_file.write(buf.data(), m_entry_size);
}

void fragment_set_log::replay(std::set<fragment_set_element>& set,
                              global_data_view& storage) {
    const auto file_size = std::filesystem::file_size(m_log_path);
    m_log_file.seekg(std::istream::beg);
    std::size_t pos = m_log_file.tellg();
    while (pos < file_size) {
        auto element = read_entry();
        switch (element.op) {
        case set_operation::INSERT:
            set.emplace(element.pointer, element.size,
                        std::string{element.prefix, element.prefix_size},
                        storage);
            break;
        default:
            throw std::invalid_argument("invalid entry in fragment set log");
        }
        pos = m_log_file.tellg();
    }
}

fragment_set_log::~fragment_set_log() {
    m_log_file.flush();
    m_log_file.close();
}

[[nodiscard]] fragment_set_log::log_entry fragment_set_log::read_entry() {
    std::array<char, m_entry_size> buf{};
    m_log_file.read(buf.data(), m_entry_size);
    log_entry entry;
    zpp::bits::in{buf, zpp::bits::size4b{}}(entry).or_throw();
    return entry;
}

void fragment_set_log::flush() {
    std::lock_guard<std::mutex> guard(m_mutex);
    m_log_file.flush();
}

} // namespace uh::cluster
