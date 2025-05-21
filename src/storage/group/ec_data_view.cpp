#include "ec_data_view.h"

#include <common/coroutines/coro_util.h>
#include <common/telemetry/log.h>
#include <common/utils/integral.h>
#include <storage/group/impl/address_utils.h>

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
        throw std::runtime_error("Invalid leader id: " +
                                 std::to_string(leader));

    auto write_size = data.size();
    auto num_chunks = div_ceil(write_size, m_stripe_size);
    auto allocation = co_await storages.at(leader)->allocate(
        num_chunks * m_chunk_size, m_chunk_size);

    if (allocation.offset % m_chunk_size != 0)
        throw std::runtime_error("Allocation result is not aligned");

    auto context = co_await boost::asio::this_coro::context;
    for (auto i = 0ul; i < num_chunks; i++) {
        allocation_t alloc{.offset = allocation.offset + i * m_chunk_size,
                           .size = m_chunk_size};

        auto data_chunk = data.subspan(i * m_stripe_size,
                                       std::min(write_size, m_stripe_size));
        write_size -= m_stripe_size;

        // TODO: Offsets are not yet transformed and distributed to each
        // storages
        auto encoded = m_rs.encode(data_chunk, m_chunk_size);
        co_await run_for_all<address, std::shared_ptr<storage_interface>>(
            m_ioc,
            [&](size_t i, auto storage) -> coro<address> {
                co_return co_await storage
                    ->write(alloc, encoded.get().at(i), offsets)
                    .continue_trace(context);
            },
            storages);
    }
    address rv;
    auto pointer = get_global_pointer(allocation.offset, 0);
    rv.push(fragment{.pointer = pointer, .size = data.size()});
    co_return rv;
}

coro<shared_buffer<>> ec_data_view::read(const uint128_t& pointer,
                                         size_t read_size) {
    auto storages = get_valid_storages();

    auto need_reconstruction =
        std::any_of(storages.begin(), storages.begin() + m_config.data_shards,
                    [](auto storage) { return storage != nullptr; });

    (void)need_reconstruction;

    address addr;
    auto p = pointer;
    while (p < pointer + read_size) {
        auto next_p = align_up_next<uint128_t>(p, m_chunk_size);
        next_p = std::min(next_p, pointer + read_size);
        auto frag = fragment{.pointer = p,
                             .size = static_cast<std::size_t>(next_p - p)};
        addr.push(frag);
        p = next_p;
    }

    auto rv = shared_buffer<>(read_size);
    co_await read_address(addr, rv);

    co_return rv;
}

coro<std::size_t> ec_data_view::read_address(const address& addr,
                                             std::span<char> buffer) {
    auto storages = m_externals.get_storage_services();
    auto need_reconstruction =
        std::any_of(storages.begin(), storages.begin() + m_config.data_shards,
                    [](auto storage) { return storage != nullptr; });

    (void)need_reconstruction;

    co_return co_await perform_for_address(
        m_ioc, addr,
        [this](uint128_t pointer) -> auto {
            return get_storage_pointer(pointer);
        },
        [buffer](size_t, std::shared_ptr<storage_interface> svc,
                 const address_info& info) -> coro<void> {
            co_await svc->read_address(info.addr, buffer, info.pointer_offsets);
        },
        storages);
}

coro<std::size_t> ec_data_view::get_used_space() { co_return 0; }

[[nodiscard]] coro<address> ec_data_view::link(const address& addr) {
    co_return address{};
}

coro<std::size_t> ec_data_view::unlink(const address& addr) { co_return 0; }

} // namespace uh::cluster::storage
