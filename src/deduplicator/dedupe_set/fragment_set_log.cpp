#include "fragment_set_log.h"
#include "common/utils/temp_file.h"
#include <fstream>

namespace uh::cluster {
fragment_set_log::fragment_set_log(std::filesystem::path log_path)
    : m_log_path(std::move(log_path)),
      m_log_file(m_log_path, std::fstream::binary | std::fstream::in |
                                 std::fstream::out | std::fstream::app),
      m_cache(SET_LOG_CACHE_SIZE, [this](std::span<char> data) {
          m_log_file.write(data.data(), data.size());
      }) {
    if (!m_log_file.is_open()) {
        throw std::runtime_error("Could not open the set log file");
    }
}

void fragment_set_log::append(const log_entry& entry) {
    std::lock_guard<std::mutex> guard(m_mutex);
    m_cache << entry;
}

void fragment_set_log::replay(std::set<fragment_set_element>& set,
                              global_data_view& storage) {
    std::lock_guard<std::mutex> guard(m_mutex);
    const auto file_size = std::filesystem::file_size(m_log_path);
    m_log_file.seekg(std::istream::beg);
    std::size_t pos = m_log_file.tellg();
    std::unordered_map<uint128_t, log_entry> log_entries;
    while (pos < file_size) {
        auto element = read_entry();
        switch (element.op) {
        case set_operation::INSERT:
            log_entries.emplace(element.pointer, element);
            break;
        case set_operation::REMOVE:
            log_entries.erase(element.pointer);
        default:
            throw std::invalid_argument("invalid entry in fragment set log");
        }
        pos = m_log_file.tellg();
    }

    temp_file tmp(m_log_path.parent_path());
    std::fstream tmp_file(tmp.get_path(), std::fstream::binary |
                                              std::fstream::out |
                                              std::fstream::trunc);
    for (const auto& [pointer, element] : log_entries) {
        set.emplace(element.pointer, element.size,
                    std::string{element.prefix, element.prefix_size}, storage);
        std::array<char, m_entry_size> buf{};
        zpp::bits::out{buf, zpp::bits::size4b{}}(element).or_throw();
        tmp_file.write(buf.data(), m_entry_size);
    }
    tmp_file.close();
    m_log_file.close();
    tmp.release_to(m_log_path);

    m_log_file.open(m_log_path, std::fstream::binary | std::fstream::in |
                                    std::fstream::out | std::fstream::app);
    if (!m_log_file.is_open()) {
        throw std::runtime_error("Could not open the set log file");
    }
}

fragment_set_log::~fragment_set_log() { flush(); }

[[nodiscard]] fragment_set_log::log_entry fragment_set_log::read_entry() {
    std::array<char, m_entry_size> buf{};
    m_log_file.read(buf.data(), m_entry_size);
    log_entry entry;
    zpp::bits::in{buf, zpp::bits::size4b{}}(entry).or_throw();
    return entry;
}

void fragment_set_log::flush() {
    std::lock_guard<std::mutex> guard(m_mutex);
    m_cache.flush();
    m_log_file.flush();
}

} // namespace uh::cluster
