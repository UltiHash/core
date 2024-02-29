#ifndef CORE_DATA_STORE_H
#define CORE_DATA_STORE_H

#include "common/utils/free_spot_manager.h"
#include <cstring>
#include <fcntl.h>
#include <list>
#include <map>
#include <memory_resource>
#include <span>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>

namespace uh::cluster {

struct data_store_config {
    std::filesystem::path working_dir;
    std::size_t min_file_size;
    std::size_t max_file_size;
    uint128_t max_data_store_size;
};

class data_store {

public:
    explicit data_store(const data_store_config& conf, std::size_t id,
                        bool adaptive = true);

    address write(std::span<char> data);

    std::size_t read(char* buffer, uint128_t pointer, size_t size);

    void remove(uint128_t pointer, size_t size);

    void sync();

    [[nodiscard]] uint128_t get_used_space() const noexcept;
    [[nodiscard]] uint128_t get_free_space() const noexcept;

    ~data_store();

private:
    struct partial_alloc_t {
        int fd{};
        std::int64_t offset{};
        std::size_t size{};
        uint128_t global_offset;
    };
    typedef std::list<partial_alloc_t> alloc_t;

    [[nodiscard]] std::pair<int, long>
    get_file_offset_pair(uint128_t pointer) const;

    [[nodiscard]] uint128_t fetch_used_space() const noexcept;

    alloc_t allocate_internal(std::size_t size);

    int add_new_file(const uint128_t& offset, long file_size);

    [[nodiscard]] static std::pair<int, uint128_t>
    parse_file_name(const std::string& filename);

    [[nodiscard]] std::string get_name(const uint128_t& offset) const;

    static bool is_data_file(const std::filesystem::path& path);

    int m_last_fd{};
    std::size_t m_last_file_data_end{};
    int m_data_id;
    data_store_config m_conf;
    free_spot_manager m_free_spot_manager;
    std::map<uint128_t, int> m_open_files;
    std::unordered_map<int, std::size_t> m_modified_files;
    uint128_t m_last_file_offset;
    std::size_t m_last_file_size;
    uint128_t m_used;
    std::shared_mutex m;
};

} // end namespace uh::cluster

#endif // CORE_DATA_STORE_H
