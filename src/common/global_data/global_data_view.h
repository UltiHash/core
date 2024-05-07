#ifndef CORE_GLOBAL_DATA_VIEW_H
#define CORE_GLOBAL_DATA_VIEW_H

#include "common/network/client.h"
#include "common/registry/services.h"
#include "common/types/scoped_buffer.h"
#include "config.h"
#include "lru_cache.h"
#include "storage/interfaces/storage_interface.h"
#include <map>

namespace uh::cluster {

class global_data_view {

public:
    /**
     * @brief Constructs a global_data view.
     *
     * The global_data_view introduces the abstraction of a flat address space
     * that fragments can be written to and read from, hiding the interaction
     * with individual storage service instances.
     *
     * @param config A constant reference to an instance of
     * global_data_view_config, providing all tunable configuration parameters.
     * @param ioc A reference to an instance of boost::asio::io_context used for
     * spawning co-routines.
     * @param storage_services A reference to an instance of
     * services<STORAGE_SERVICE> used for service discovery.
     */
    explicit global_data_view(const global_data_view_config& config,
                              boost::asio::io_context& ioc,
                              services<storage_interface>& storage_services);

    /**
     * @brief Sends write request to a storage service instance, does not
     * guarantee persistence.
     *
     * Sends write request to a storage service instance. Upon successful
     * completion of the request, the fragment (#data) and its resulting address
     * are stored in the L1 cache. CAUTION: writes are only guaranteed to be
     * persistent after sync has been called.
     *
     * @param data A constant reference to a std::string_view holding the data
     * to be written.
     * @return An #address the data has been written to.
     */
    address write(const std::string_view& data);

    /**
     * @brief Retrieves fragment from storage services.
     *
     * The L2 read cache is consulted to see if it contains the requested
     * fragment. Otherwise, a read request is issued to the storage service
     * instance serving the address range the provided #pointer is in.
     * - If the requested fragment can be served by a storage service, the
     * fragment and its address are (re)-inserted into both the L1 and L2
     * read caches.
     * - If no storage service can serve the fragment, a std::runtime_error
     * exception is thrown
     *
     * The L1 read cache only contains up to the first 128 byte of a fragment,
     * but holds a much larger number of entries and can thus be used for
     * efficient sample comparison, whereas the L2 read cache contains the
     * entire content of a fragment at the price of a smaller cache capacity.
     * Together, both caches are drastically reducing the number of read
     * requests that need to be issued to the storage service instances.
     *
     * @param pointer A constant reference to a uint128_t, specifying the
     * location of the fragment.
     * @param size A constant size_t specifying the size of the fragment.
     * @return A shared_buffer<char> containing the fragment data.
     */
    shared_buffer<char> read_fragment(const uint128_t& pointer,
                                      const size_t size);

    /**
     * @brief Retrieves the contents of an entire address from storage services.
     *
     * Retrieves content of an entire address by scattering read requests for
     * each fragment to storage service instances for improved read performance.
     * This method entirely bypasses the read caches.
     *
     * @param[out] buffer A char buffer that the retrieved data is written to.
     * @param[in] addr An constant reference to the address instance data should
     * be read from.
     * @return The number of bytes read.
     */
    coro <std::size_t> read_address(char* buffer, const address& addr);

    /**
     * @brief Must be called on all addresses returned #write to ensure their
     * persistence.
     *
     * Data written using the #write method is only guaranteed to be persistent
     * after calling this method on the resulting address.
     *
     * @param addr The address of all data to be synced to persistent storage.
     */
    void sync(const address& addr);

    /**
     * @brief Computes used space across all available storage service
     * instances.
     * @return The used space across all available storage service instances.
     */
    [[nodiscard]] uint128_t get_used_space();

    /**
     * @brief Computes available space across all available storage service
     * instances.
     * @return The available space across all available storage service
     * instances.
     */
    [[nodiscard]] uint128_t get_available_space();

    /**
     * @brief Provides access to the I/O context used by the global_data_view
     * @return A reference to the boost::asio::io_context used by the
     * global_data_view
     */
    [[nodiscard]] boost::asio::io_context& get_executor() const;

    /**
     * @brief Returns the configured number of connections maintained to each
     * storage service instance.
     * @return The configured number of connections maintained to each storage
     * service instance.
     */
    [[nodiscard]] std::size_t
    get_storage_service_connection_count() const noexcept;

private:
    boost::asio::io_context& m_io_service;
    services<storage_interface>& m_storage_services;
    global_data_view_config m_config;
    lru_cache<uint128_t, shared_buffer<char>> m_cache_l2;
};

} // end namespace uh::cluster

#endif // CORE_GLOBAL_DATA_VIEW_H
