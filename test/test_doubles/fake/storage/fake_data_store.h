#pragma once

#include "common/types/address.h"
#include "storage/interfaces/data_store.h"

#include <cstring>
#include <filesystem>

namespace uh::cluster {

class fake_data_store : public abstract_data_store {

public:
    fake_data_store(data_store_config conf,
                    const std::filesystem::path& working_dir,
                    uint32_t service_id, uint32_t data_store_id);

    address write(const std::string_view& data);
    void manual_write(uint64_t internal_pointer, const std::string_view& data);
    void manual_read(uint64_t pointer, size_t size, char* buffer);
    std::size_t read(char* buffer, const uint128_t& pointer, size_t size);
    std::size_t read_up_to(char* buffer, const uint128_t& pointer, size_t size);
    address link(const address& addr);
    size_t unlink(const address& addr);
    [[nodiscard]] size_t get_used_space() const noexcept;
    [[nodiscard]] size_t get_available_space() const noexcept;

    size_t id() const noexcept;

    ~fake_data_store();

private:
    const uint32_t m_storage_id;
    const uint32_t m_data_store_id;
    const std::filesystem::path m_root;
    const std::string m_datafile = "data.backup";
    const std::string m_refcountfile = "refcount.backup";

    data_store_config m_conf;

    std::vector<char> m_data;
    std::unordered_map<fragment, int> m_refcounter;

    std::mutex m_mutex;
};

} // end namespace uh::cluster
