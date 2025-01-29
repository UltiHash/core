#pragma once

#include "common/caches/lru_cache.h"
#include "common/etcd/ec_groups/ec_get_handler.h"
#include "common/etcd/ec_groups/ec_group_maintainer.h"
#include "common/etcd/ec_groups/ec_load_balancer.h"
#include "common/etcd/service_discovery/service_maintainer.h"
#include "common/types/scoped_buffer.h"
#include "config.h"
#include "global_data_view.h"
#include "storage/interfaces/remote_storage.h"

namespace uh::cluster {

class default_global_data_view : public storage_interface {

public:
    /**
     * @brief Constructs a global_data view.
     *
     * The default_global_data_view introduces the abstraction of a flat
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
    default_global_data_view(const global_data_view_config& config,
                             boost::asio::io_context& ioc, etcd_manager& etcd);

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
    coro<address> write(context& ctx, std::span<const char> data,
                        const std::vector<std::size_t>& offsets) override;

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
    coro<std::size_t> read(context& ctx, const address& addr,
                           std::span<char> buffer) override;

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
    coro<address> link(context& ctx, const address& addr) override;

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
    coro<std::size_t> unlink(context& ctx, const address& addr) override;

    /**
     * @brief Computes used space across all available storage service
     * instances.
     * @param ctx open telemetry context
     * @return The used space across all available storage service instances.
     */
    coro<std::size_t> get_used_space(context& ctx) override;

    ~default_global_data_view() noexcept;

private:
    boost::asio::io_context& m_io_service;
    global_data_view_config m_config;

    service_maintainer<distributed_storage, remote_factory>
        m_service_maintainer;
    ec_group_maintainer m_ec_maintainer;
    ec_load_balancer m_load_balancer;
    ec_get_handler m_basic_getter;
};

} // end namespace uh::cluster
