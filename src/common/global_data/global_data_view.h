#ifndef CORE_GLOBAL_DATA_VIEW_H
#define CORE_GLOBAL_DATA_VIEW_H

#include "common/network/client.h"
#include "common/registry/services.h"
#include "common/types/shared_buffer.h"
#include "common/utils/worker_pool.h"
#include "lru_cache.h"
#include <map>

namespace uh::cluster {

struct global_data_view_config {
    std::size_t storage_service_connection_count = 16;
    std::size_t read_cache_capacity_l1 = 8000000ul;
    std::size_t read_cache_capacity_l2 = 4000ul;
    std::size_t l1_sample_size = 128ul;
    uint128_t max_data_store_size = DATASTORE_MAX_SIZE;
};

class global_data_view {

public:
    explicit global_data_view(const global_data_view_config& config,
                              boost::asio::io_context& ioc,
                              worker_pool& workers,
                              services<STORAGE_SERVICE>& storage_services);

    address write(const std::string_view& data);

    shared_buffer<char> cached_sample(const uint128_t pointer,
                                      const size_t size);

    shared_buffer<char> read(const uint128_t& pointer, const size_t size);

    std::size_t read_address(char* buffer, const address& addr);

    coro<void> remove(const uint128_t pointer, const size_t size);

    void sync(const address& addr);

    [[nodiscard]] uint128_t get_used_space();

    [[nodiscard]] boost::asio::io_context& get_executor() const;

    [[nodiscard]] std::size_t l1_cache_sample_size() const noexcept;

    [[nodiscard]] std::size_t
    get_storage_service_connection_count() const noexcept;

private:
    boost::asio::io_context& m_io_service;
    worker_pool& m_workers;
    services<STORAGE_SERVICE>& m_storage_services;
    global_data_view_config m_config;

    lru_cache<uint128_t, shared_buffer<char>> m_cache_l1;
    lru_cache<uint128_t, shared_buffer<char>> m_cache_l2;
};

} // end namespace uh::cluster

#endif // CORE_GLOBAL_DATA_VIEW_H
