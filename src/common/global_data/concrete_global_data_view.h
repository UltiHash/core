#pragma once

#include "common/caches/lru_cache.h"
#include "common/etcd/ec_groups/ec_get_handler.h"
#include "common/etcd/ec_groups/ec_group_maintainer.h"
#include "common/etcd/ec_groups/ec_load_balancer.h"
#include "common/etcd/service_discovery/service_maintainer.h"
#include "common/types/scoped_buffer.h"
#include "config.h"
#include "global_data_view.h"

namespace uh::cluster {

class concrete_global_data_view : public global_data_view {

public:
    /**
     * @brief Constructs a global_data view.
     *
     * The concrete_global_data_view introduces the abstraction of a flat
     * address space that fragments can be written to and read from, hiding the
     * interaction with individual storage service instances.
     *
     * @param config A constant reference to an instance of
     * global_data_view_config, providing all tunable configuration parameters.
     * @param ioc A reference to an instance of boost::asio::io_context used for
     * spawning co-routines.
     * @param storage_maintainer A reference to an instance of
     * service maintainer used for service discovery.
     */
    explicit concrete_global_data_view(
        const global_data_view_config& config, boost::asio::io_context& ioc,
        service_maintainer<storage_interface>& storage_maintainer);

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
    coro<address> write(context& ctx, const std::string_view& data,
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
    coro<shared_buffer<>> read(context& ctx, const uint128_t& pointer,
                               size_t size);

    /**
     * @brief Retrieves fragment from storage services.
     *
     * The L2 read cache is consulted to see if it contains the requested
     * fragment. Otherwise, a read request is issued to the storage service
     * instance serving the address range the provided #pointer is in.
     * - If the requested fragment can be served by a storage service, the
     * fragment and its address are (re)-inserted into the L2
     * read cache.
     * - If no storage service can serve the fragment, a std::runtime_error
     * exception is thrown
     *
     * The L2 read cache contains the
     * entire content of a fragment at the price of a smaller cache capacity.
     *
     * @param ctx traces context
     * @param pointer A constant reference to a uint128_t, specifying the
     * location of the fragment.
     * @param size A size_t specifying the size of the fragment.
     * @return A shared_buffer<char> containing the fragment data.
     */
    shared_buffer<char> read_fragment(context& ctx, const uint128_t& pointer,
                                      size_t size);

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
    coro<std::size_t> read_address(context& ctx, char* buffer,
                                   const address& addr);

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
    [[nodiscard]] coro<address> link(context& ctx, const address& addr);

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
    coro<std::size_t> unlink(context& ctx, const address& addr);

    /**
     * @brief Computes used space across all available storage service
     * instances.
     * @param ctx open telemetry context
     * @return The used space across all available storage service instances.
     */
    coro<std::size_t> get_used_space(context& ctx);

    /**
     * @brief Provides access to the I/O context used by the
     * concrete_global_data_view
     * @param c open telemetry context
     * @return A reference to the boost::asio::io_context used by the
     * concrete_global_data_view
     */
    [[nodiscard]] boost::asio::io_context& get_executor() const;

    /**
     * @brief Returns the configured number of connections maintained to each
     * storage service instance.
     * @param c open telemetry context
     * @return The configured number of connections maintained to each storage
     * service instance.
     */
    [[nodiscard]] std::size_t
    get_storage_service_connection_count() const noexcept;

    ~concrete_global_data_view() noexcept;

private:
    boost::asio::io_context& m_io_service;
    global_data_view_config m_config;
    lru_cache<uint128_t, shared_buffer<char>> m_cache_l2;

    service_maintainer<storage_interface>& m_service_maintainer;
    ec_group_maintainer m_ec_maintainer;
    ec_load_balancer m_load_balancer;
    ec_get_handler m_basic_getter;
};

} // end namespace uh::cluster
