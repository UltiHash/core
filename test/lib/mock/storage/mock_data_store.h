#pragma once

#include "common/types/address.h"
#include "storage/interfaces/data_store.h"

#include <atomic>
#include <cstring>
#include <filesystem>
#include <mutex>

namespace uh::cluster {

class mock_data_store : public data_store {

public:
    mock_data_store(data_store_config conf,
                    const std::filesystem::path& working_dir,
                    uint32_t service_id, uint32_t data_store_id);

    allocation_t allocate(std::size_t size, std::size_t alignment = 1);
    address write(const allocation_t allocation, std::span<const char> data,
                  const std::vector<std::size_t>& offsets);
    std::size_t read(const std::size_t pointer, std::span<char> buffer);
    address link(const address& addr);
    size_t unlink(const address& addr);
    [[nodiscard]] size_t get_used_space() const noexcept;
    [[nodiscard]] size_t get_available_space() const noexcept;
    [[nodiscard]] std::size_t get_write_offset() const noexcept;
    void clear(); // for testing

    size_t id() const noexcept;

    ~mock_data_store();

private:
    const uint32_t m_storage_id;
    const uint32_t m_data_store_id;
    const std::filesystem::path m_root;
    const std::string m_datafile = "data.backup";
    const std::string m_refcountfile = "refcount.backup";

    std::atomic<size_t> m_current_offset{0};

    data_store_config m_conf;

    std::vector<char> m_data;
    std::unordered_map<fragment, int> m_refcounter;
    std::mutex m_mutex;
};

} // end namespace uh::cluster
