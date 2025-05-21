#include "ec_data_view.h"

#include <common/coroutines/coro_util.h>
#include <common/telemetry/log.h>
#include <common/utils/integral.h>
#include <unordered_set>

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
      m_rs{config.data_shards, config.parity_shards, m_chunk_size},
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
        auto encoded = m_rs.encode(data_chunk);
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
    rv.emplace_back(pointer, data.size());
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
    auto end = pointer + read_size;
    auto current_p = pointer;

    {
        auto size =
            std::min(align_up_next<uint128_t>(current_p, m_chunk_size), end) -
            current_p;
        addr.emplace_back(current_p, static_cast<std::size_t>(size));
        current_p += size;
    }

    while (current_p < end) {
        auto size = std::min(current_p + m_chunk_size, end) - current_p;
        addr.emplace_back(current_p, static_cast<std::size_t>(size));
        current_p += size;
    }

    auto rv = shared_buffer<>(read_size);
    co_await read_address(addr, rv);

    co_return rv;
}

coro<std::unordered_map<std::size_t, bool>> ec_data_view::read_from_storages(
    std::unordered_map<std::size_t, address_info> addr_map,
    std::span<char> buffer) {
    auto size = 0ul;
    co_return co_await run_for_all<bool>(
        m_ioc,
        [&size, &storage_index = m_externals.get_storage_index(),
         buffer](std::size_t id, const address_info& info) -> coro<bool> {
            size += info.addr.data_size();
            try {
                auto storage = storage_index.at(id);
                if (storage == nullptr)
                    co_return false;

                co_await storage->read_address(info.addr, buffer,
                                               info.pointer_offsets);
            } catch (...) {
                LOG_ERROR() << "Failed to read address";
                co_return false;
            }
            co_return true;
        },
        addr_map);
}
coro<std::size_t> ec_data_view::read_address(const address& addr,
                                             std::span<char> buffer) {

    auto addr_map =
        extract_node_address_map(addr, [this](uint128_t pointer) -> auto {
            return get_storage_pointer(pointer);
        });

    auto success_map = co_await read_from_storages(addr_map, buffer);

    auto success = std::ranges::all_of(success_map | std::views::values,
                                       [](auto success) { return success; });
    if (!success) {
        auto storages = m_externals.get_storage_services();
        auto num_valid_storages = std::ranges::count_if(
            storages, [](auto& s) { return s != nullptr; });

        if (num_valid_storages < m_config.data_shards) {
            throw std::runtime_error(
                "Failed to read address: there's not enough "
                "valid storages");
        }

        auto need_reconstruction = std::any_of(
            storages.begin(), storages.begin() + m_config.data_shards,
            [](auto storage) { return storage != nullptr; });

        if (not need_reconstruction) {
            throw std::runtime_error(
                "Failed to read address: but don't know why read_address for "
                "storages was failed");
        }

        // get needed stripe ids from failed read_address operations
        std::unordered_map<uint64_t, std::vector<fragment>> stripe_map;
        for (auto& [id, success] : success_map) {
            if (not success) {
                auto& addr = addr_map[id].addr;
                for (auto& frag : addr.fragments) {
                    (void)frag;
                    auto global_pointer = get_global_pointer(frag.pointer, id);
                    auto stripe_id = static_cast<uint64_t>(div_floor(
                        global_pointer, static_cast<uint128_t>(m_stripe_size)));
                    stripe_map[stripe_id].push_back(frag);
                }
            }
        }

        // Repeat the following steps for each stripe:
        for (auto& [stripe_id, frags] : stripe_map) {
            address addr;
            addr.emplace_back(m_chunk_size * stripe_id, m_chunk_size);
            // storages
            // std::vector stats(6, data_stat::valid);
            // stats[1] = data_stat::lost;
            // stats[3] = data_stat::lost;
            // m_rs.recover(shards, stats);
            // TODO: 2. Read a chunk on the enough number of shards
            // TODO: 3. Reconstruct needed data
            // TODO: 4. Copy recustructed data to the buffer
        }
    }

    co_return addr.data_size();
}

coro<std::size_t> ec_data_view::get_used_space() { co_return 0; }

[[nodiscard]] coro<address> ec_data_view::link(const address& addr) {
    co_return address{};
}

coro<std::size_t> ec_data_view::unlink(const address& addr) { co_return 0; }

} // namespace uh::cluster::storage
