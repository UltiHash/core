#include "ec_data_view.h"

#include <common/coroutines/coro_util.h>
#include <common/telemetry/log.h>
#include <type_traits>

namespace {
template <typename T> constexpr T div_ceil(T x, T y) {
    static_assert(std::is_integral<T>::value,
                  "div_ceil only supports integral types");
    return (x + y - 1) / y;
}
} // namespace

namespace uh::cluster::storage {
ec_data_view::ec_data_view(boost::asio::io_context& ioc, etcd_manager& etcd,
                           std::size_t group_id, group_config config,
                           std::size_t service_connections)
    : m_ioc(ioc),
      m_config{config},
      m_block_size{m_config.chunk_size_kib * m_config.data_shards},
      m_rs{config.data_shards, config.parity_shards},
      m_externals(
          etcd, group_id, config.storages,
          service_factory<storage_interface>(ioc, service_connections)) {}

coro<address> ec_data_view::write(std::span<const char> data,
                                  const std::vector<std::size_t>& offsets) {

    if (*m_externals.get_group_state() != group_state::HEALTHY)
        throw std::runtime_error("group state should be healthy");

    auto storages = m_externals.get_storage_services();
    auto leader = *m_externals.get_leader();

    if (leader < 0 or leader >= (candidate::id_t)m_config.storages)
        throw std::runtime_error("Read size must be larger than zero");

    auto size = data.size();
    auto num_chunks = div_ceil(size, m_block_size);
    auto allocation = co_spawn(m_ioc,
                               storages.at(leader)->allocate(
                                   num_chunks * m_config.chunk_size_kib),
                               boost::asio::use_future)
                          .get();

    if (allocation.offset % m_config.chunk_size_kib != 0)
        throw std::runtime_error("Allocation result is not aligned");

    address addr;
    for (auto i = 0ul; i < num_chunks; i++) {
        auto offset = i * m_config.chunk_size_kib;
        allocation_t alloc{.offset = allocation.offset + offset,
                           .size = m_config.chunk_size_kib};
        auto data_chunk = data.subspan(offset, m_config.chunk_size_kib);

        auto encoded = m_rs.encode(data_chunk);
        auto context = co_await boost::asio::this_coro::context;
        auto res =
            co_await run_for_all<address, std::shared_ptr<storage_interface>>(
                m_ioc,
                [&context, &encoded, &offsets,
                 &alloc](size_t i, auto n) -> coro<address> {
                    co_return co_await n
                        ->write(alloc, encoded.get().at(i), offsets)
                        .continue_trace(context);
                },
                storages);

        for (const auto& a : res) {
            addr.append(a);
        }
    }

    co_return addr;
}

coro<shared_buffer<>> ec_data_view::read(const uint128_t& pointer,
                                         size_t size) {
    // - m_externals.get_storage_services();
    // - m_externals.get_storage_states()
    auto storages = m_externals.get_storage_services();
    auto states = m_externals.get_storage_states();
    (void)storages;
    (void)states;
    // 1. In case every data shards are ASSIGNED state, read from them without
    //    reconstruction
    // 1. Else if some of the data shards are not ASSIGNED,
    //    1.1. If number of ASSIGNED shards is less than data_shards, throw
    //    1.2. If not, read m shards, as many data shards as possible, and
    //         reconstruct
    // 3. Send data shards
    co_return shared_buffer<>{};
}

coro<std::size_t> ec_data_view::read_address(const address& addr,
                                             std::span<char> buffer) {
    co_return 0;
}

coro<std::size_t> ec_data_view::get_used_space() { co_return 0; }

[[nodiscard]] coro<address> ec_data_view::link(const address& addr) {
    co_return address{};
}

coro<std::size_t> ec_data_view::unlink(const address& addr) { co_return 0; }

} // namespace uh::cluster::storage
