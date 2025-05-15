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
      m_stripe_size{m_config.stripe_size_kib * 1_KiB},
      m_chunk_size{[&]() {
          if (m_stripe_size % m_config.data_shards != 0)
              throw std::runtime_error(
                  "Stripe size must be divisible by data shards");
          return m_stripe_size / m_config.data_shards;
      }()},
      m_rs{config.data_shards, config.parity_shards},
      m_externals(
          etcd, group_id, config.storages,
          service_factory<storage_interface>(ioc, service_connections)) {}

coro<address> ec_data_view::write(std::span<const char> data,
                                  const std::vector<std::size_t>& offsets) {

    if (*m_externals.get_group_state() != group_state::HEALTHY)
        throw std::runtime_error("group state should be healthy: " +
                                 serialize(*m_externals.get_group_state()));

    auto storages = m_externals.get_storage_services();
    auto leader = *m_externals.get_leader();

    if (leader < 0 or leader >= (candidate::id_t)m_config.storages)
        throw std::runtime_error("Read size must be larger than zero");

    auto write_size = data.size();
    auto num_chunks = div_ceil(write_size, m_stripe_size);
    auto allocation =
        co_await storages.at(leader)->allocate(num_chunks * m_chunk_size);

    if (allocation.offset % m_chunk_size != 0)
        throw std::runtime_error("Allocation result is not aligned");

    auto context = co_await boost::asio::this_coro::context;
    address rv;
    for (auto i = 0ul; i < num_chunks; i++) {
        allocation_t alloc{.offset = allocation.offset + i * m_chunk_size,
                           .size = m_chunk_size};

        auto data_chunk = data.subspan(i * m_stripe_size,
                                       std::min(write_size, m_stripe_size));
        write_size -= m_stripe_size;

        auto encoded = m_rs.encode(data_chunk);
        auto res =
            co_await run_for_all<address, std::shared_ptr<storage_interface>>(
                m_ioc,
                [&](size_t i, auto storage) -> coro<address> {
                    co_return co_await storage
                        ->write(alloc, encoded.get().at(i), offsets)
                        .continue_trace(context);
                },
                storages);

        auto& addr0 = res[0];
        for (auto i = 0ul; i < addr0.size(); i++) {
            auto pointer = pointer_traits::get_global_pointer(
                addr0.get(i).pointer.get_low() * m_config.data_shards,
                m_config.id, 0, 0);

            std::size_t frag_size = 0;
            for (auto j = 0ul; j < res.size(); j++) {
                frag_size += res[j].get(i).size;
            }

            rv.push(fragment{.pointer = pointer, .size = frag_size});
        }
    }
    co_return rv;
}

coro<shared_buffer<>> ec_data_view::read(const uint128_t& pointer,
                                         size_t read_size) {
    auto storages = m_externals.get_storage_services();
    auto states = m_externals.get_storage_states();

    size_t count = 0, valid_datashards_count = 0;
    for (auto i = 0ul; i < m_config.data_shards; ++i) {
        if (storages[i] != nullptr && *states[i] == storage_state::ASSIGNED) {
            ++count;
            if (i < m_config.data_shards)
                ++valid_datashards_count;
        } else
            storages[i] = nullptr;
    }

    if (count < m_config.data_shards)
        throw std::runtime_error("Not enough shards to reconstruct data: " +
                                 std::to_string(count));

    auto context = co_await boost::asio::this_coro::context;
    auto num_chunks = div_ceil(read_size, m_stripe_size);
    auto single_shard_size = num_chunks * m_chunk_size;

    if (pointer.get_low() % m_config.data_shards != 0)
        throw std::runtime_error("Pointer is not aligned");

    uint128_t shard_pointer = pointer.get_low() / m_config.data_shards;

    auto res = co_await run_for_all<shared_buffer<>,
                                    std::shared_ptr<storage_interface>>(
        m_ioc,
        [&](size_t i, auto storage) -> coro<shared_buffer<>> {
            if (storage == nullptr) {
                co_return shared_buffer<>(single_shard_size);
            } else {
                co_return co_await storage
                    ->read(shard_pointer, single_shard_size)
                    .continue_trace(context);
            }
        },
        storages);

    if (valid_datashards_count != m_config.data_shards) {
        // TODO: do reconstruction
    }

    auto rv = shared_buffer<>(read_size);

    auto dest = rv.data();
    for (auto i = 0ul; i < num_chunks; ++i) {
        for (auto j = 0ul; j < m_config.data_shards; ++j) {
            std::memcpy(dest, res[j].data() + i * m_chunk_size,
                        std::min(read_size, m_chunk_size));
            dest += m_chunk_size;
            read_size -= m_chunk_size;
        }
    }

    co_return rv;
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
