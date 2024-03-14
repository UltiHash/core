#include "fragment_set_log.h"

namespace uh::cluster {
fragment_set_log::fragment_set_log(std::filesystem::path log_path)
    : m_log_path(std::move(log_path)),
      m_log_file(get_log_file(m_log_path)) {
    if (m_log_file <= 0) {
        perror(m_log_path.c_str());
        throw std::runtime_error("Could not open the transaction log file");
    }
    lseek(m_log_file, 0, SEEK_END);
}

void fragment_set_log::append(const log_entry& entry) {

    char buf[sizeof(log_entry)];
    serialize_entry(entry, buf);

    if (sizeof buf != ::write(m_log_file, buf, sizeof buf)) [[unlikely]] {
        throw std::runtime_error("Could not write into the set log file");
    }
    fsync(m_log_file);
}

void fragment_set_log::replay(std::set<fragment_set_element>& set,
                              global_data_view& storage) {
    const auto file_size = std::filesystem::file_size(m_log_path);
    size_t offset = 0;
    if (0 != lseek(m_log_file, 0, SEEK_SET)) [[unlikely]] {
        throw std::runtime_error("Could not seek the set log file");
    }
    while (offset < file_size) {
        auto element = read_entry();
        switch (element.op) {
        case set_operation::INSERT:
            set.emplace(element.pointer, element.size, element.prefix, storage);
            break;
        default:
            throw std::invalid_argument("invalid entry in fragment set log");
        }
        offset += sizeof(log_entry);
    }

    lseek(m_log_file, 0, SEEK_END);
}

fragment_set_log::~fragment_set_log() {
    fsync(m_log_file);
    close(m_log_file);
}

int fragment_set_log::get_log_file(const std::filesystem::path& path) {
    std::filesystem::create_directories(path.parent_path());
    if (std::filesystem::exists(path)) {
        return ::open(path.c_str(), O_RDWR | O_APPEND);
    } else {
        return ::open(path.c_str(), O_RDWR | O_APPEND | O_CREAT,
                      S_IWUSR | S_IRUSR);
    }
}

void fragment_set_log::serialize_entry(const log_entry& entry, char* buf) {
    buf[0] = entry.op;
    size_t offset = sizeof entry.op;
    std::memcpy(buf + offset, entry.pointer.get_data(), sizeof entry.pointer);
    offset += sizeof entry.pointer;
    std::memcpy(buf + offset, &entry.size, sizeof entry.size);
    offset += sizeof entry.size;
    std::memcpy(buf + offset, entry.prefix.get_data(), sizeof entry.prefix);
}

fragment_set_log::log_entry
fragment_set_log::deserialize_entry(const char* buf) {
    log_entry entry;
    entry.op = static_cast<set_operation>(buf[0]);
    size_t offset = sizeof entry.op;
    memcpy(entry.pointer.ref_data(), buf + offset, sizeof entry.pointer);
    offset += sizeof entry.pointer;
    memcpy(&entry.size, buf + offset, sizeof entry.size);
    offset += sizeof entry.size;
    memcpy(entry.prefix.ref_data(), buf + offset, sizeof entry.prefix);

    return entry;
}

[[nodiscard]] fragment_set_log::log_entry fragment_set_log::read_entry() {
    char buf[sizeof(log_entry)];
    if (const auto rc = ::read(m_log_file, buf, sizeof buf); rc != sizeof buf)
        [[unlikely]] {
        perror("Read error");
        throw std::runtime_error("could not read the fragment set element from "
                                 "the fragment set log file");
    }

    return deserialize_entry(buf);
}

} // namespace uh::cluster
