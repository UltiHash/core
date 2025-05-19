#pragma once

#include "config.h"

#include <common/coroutines/coro_util.h>
#include <common/ec/reedsolomon_c.h>
#include <common/etcd/service_discovery/service_maintainer.h>
#include <common/etcd/service_discovery/storage_index.h>
#include <common/types/scoped_buffer.h>
#include <storage/group/externals.h>
#include <storage/interfaces/data_view.h>

namespace uh::cluster::storage {

class ec_data_view : public data_view {
    friend class cache;

public:
    /**
     * @brief Constructs a global_data view.
     *
     * The default_global_data_view introduces the abstraction of a flat
     * address space that fragments can be written to and read from, hiding the
     * interaction with individual storage service instances.
     *
     * @param ioc A reference to an instance of boost::asio::io_context used for
     * spawning co-routines.
     * @param storage_maintainer A reference to an instance of
     * service maintainer used for service discovery.
     */
    explicit ec_data_view(boost::asio::io_context& ioc, etcd_manager& etcd,
                          std::size_t group_id, group_config group_config,
                          std::size_t service_connections);

    /**
     * @brief Sends write request to a storage service instance, does not
     * guarantee persistence.
     *
     * Sends write request to a storage service instance. Upon successful
     * completion of the request, the fragment (#data) and its resulting address
     * are stored in the L1 cache. CAUTION: writes are only guaranteed to be
     * persistent after sync has been called.
     *
     * @param ctx traces context
     * @param data A constant reference to a std::string_view holding the data
     * to be written.
     * @return An #address the data has been written to.
     */
    coro<address> write(std::span<const char> data,
                        const std::vector<std::size_t>& offsets);

    /**
     * @brief reads the data starting from pointer, up to the given size.
     * It is allowed to return data that is smaller than the requested size if
     * there is no more data left in the data store file.
     *
     * @param ctx traces context
     * @param pointer A constant reference to a uint128_t, specifying the
     * location of the size
     * @param size A size_t specifying the size of the fragment.
     * @return
     */
    coro<shared_buffer<>> read(const uint128_t& pointer, size_t size);

    /**
     * @brief Retrieves the contents of an entire address from storage services.
     *
     * Retrieves content of an entire address by scattering read requests for
     * each fragment to storage service instances for improved read performance.
     * This method entirely bypasses the read caches.
     *
     * @param ctx open telemetry context
     * @param[out] buffer A char buffer that the retrieved data is written to.
     * @param[in] addr An constant reference to the address instance data should
     * be read from.
     * @return The number of bytes read.
     */
    coro<std::size_t> read_address(const address& addr, std::span<char> buffer);

    /**
     * @brief registers a reference to a storage region to claim co-ownership
     * of data.
     *
     * In the sense of optimistic concurrency control, this call
     * tries to increase the reference count of a storage region. If due to
     * a data race, data in the specified region has already been deleted,
     * an address with the affected regions is returned to enable upstream
     * handling of the issue.
     *
     * @param ctx traces context
     * @param addr The address specifying the storage regions to be referenced.
     * @return An address specifying all storage regions within #addr that have
     *
     * already been deleted and therefore can no longer be referenced.
     */
    [[nodiscard]] coro<address> link(const address& addr);

    /**
     * @brief un-registers a reference to a storage region to release
     * co-ownership of data.
     *
     * @param ctx traces context
     * @param addr The address specifying the storage regions to be
     * un-referenced.
     * @return number of bytes freed in response to removing references.
     * In case of an error, std::numeric_limits<std::size_t>::max() is returned.
     */
    coro<std::size_t> unlink(const address& addr);

    /**
     * @brief Computes used space across all available storage service
     * instances.
     * @param ctx open telemetry context
     * @return The used space across all available storage service instances.
     */
    coro<std::size_t> get_used_space();

    uint128_t group_pointer(size_t storage_id, uint64_t storage_pointer) {
        return ((uint128_t)(storage_pointer / m_chunk_size) * m_stripe_size) +
               (storage_pointer % m_chunk_size) +
               ((uint128_t)m_chunk_size * storage_id);
    }

    std::pair<std::size_t, uint64_t> storage_pointer(uint128_t group_pointer) {
        std::size_t group_mod = group_pointer % m_stripe_size;
        std::size_t storage_id = group_mod / m_chunk_size;
        std::size_t storage_ptr =
            (group_pointer / m_stripe_size) * m_chunk_size +
            (group_mod - storage_id * m_chunk_size);
        return {storage_id, storage_ptr};
    }

private:
    boost::asio::io_context& m_ioc;
    group_config m_config;
    std::size_t m_stripe_size;
    std::size_t m_chunk_size;
    reedsolomon_c m_rs;
    externals_subscriber m_externals;

    struct address_info {
        address addr;
        std::vector<size_t> pointer_offsets;
    };

    struct node_address_info {
        std::unordered_map<std::size_t, address_info> node_info_map;
        size_t data_size;
    };

    node_address_info extract_node_address_map(const address& addr) {
        node_address_info info;
        size_t offset = 0;
        for (size_t i = 0; i < addr.size(); ++i) {
            const auto frag = addr.get(i);
            auto [storage_id, storage_poniter] = storage_pointer(frag.pointer);
            // frag.pointer / m_chunk_size / m_config.data_shards
            auto& node_pos = info.node_info_map[storage_id];
            auto& node_address = node_pos.addr;
            node_address.push(frag);
            node_pos.pointer_offsets.emplace_back(offset);
            offset += frag.size;
        }

        info.data_size = offset;
        return info;
    }

    coro<size_t> perform_for_address(
        const address& addr,
        const std::vector<std::shared_ptr<storage_interface>>& storages,
        boost::asio::io_context& ioc,
        std::function<coro<void>(size_t, std::shared_ptr<storage_interface>,
                                 const address_info&)>
            fn) {

        auto info = extract_node_address_map(addr);

        auto context = co_await boost::asio::this_coro::context;

        co_await run_for_all<void, std::shared_ptr<storage_interface>>(
            m_ioc,
            [&](size_t i, auto storage) -> coro<void> {
                co_return co_await fn(i, storage, info.node_info_map[i])
                    .continue_trace(context);
            },
            storages);

        co_return info.data_size;
    }
};

} // namespace uh::cluster::storage
