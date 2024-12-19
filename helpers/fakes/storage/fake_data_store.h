#pragma once

#include "common/types/address.h"
#include "storage/interfaces/data_store.h"

#include <atomic>
#include <cstring>
#include <filesystem>
#include <mutex>

namespace uh::cluster {

struct fragment_hash {
    std::size_t operator()(const fragment& f) const {
        uint64_t high = static_cast<uint64_t>(f.pointer.get_low());
        uint64_t low = static_cast<uint64_t>(f.pointer.get_high());

        std::size_t h1 = std::hash<uint64_t>()(high);
        std::size_t h2 = std::hash<uint64_t>()(low);
        std::size_t h3 = std::hash<std::size_t>()(f.size);

        std::size_t seed = h1;
        auto hash_combine = [&](std::size_t& s, std::size_t v) {
            s ^= v + 0x9e3779b97f4a7c16ULL + (s << 6) + (s >> 2);
        };
        hash_combine(seed, h2);
        hash_combine(seed, h3);

        return seed;
    }
};

class fake_data_store : public abstract_data_store {

public:
    fake_data_store(data_store_config conf,
                    const std::filesystem::path& working_dir,
                    uint32_t service_id, uint32_t data_store_id);

    address write(const std::string_view& data,
                  const std::vector<std::size_t>& offsets);
    void manual_write(uint64_t internal_pointer, const std::string_view& data);
    void manual_read(uint64_t pointer, size_t size, char* buffer);
    std::size_t read(char* buffer, const uint128_t& pointer, size_t size);
    std::size_t read_up_to(char* buffer, const uint128_t& pointer, size_t size);
    address link(const address& addr);
    size_t unlink(const address& addr);
    [[nodiscard]] size_t get_used_space() const noexcept;
    [[nodiscard]] size_t get_available_space() const noexcept;
    void clear(); // for testing

    size_t id() const noexcept;

    ~fake_data_store();

private:
    const uint32_t m_storage_id;
    const uint32_t m_data_store_id;
    const std::filesystem::path m_root;
    const std::string m_datafile = "data.backup";
    const std::string m_refcountfile = "refcount.backup";

    std::atomic<size_t> m_current_offset{0};

    data_store_config m_conf;

    std::vector<char> m_data;
    std::unordered_map<fragment, int, fragment_hash> m_refcounter;
    std::mutex m_mutex;
};

} // end namespace uh::cluster
