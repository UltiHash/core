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
    auto num_stripes = div_ceil(write_size, m_stripe_size);
    auto allocation = co_await storages.at(leader)->allocate(
        num_stripes * m_chunk_size, m_chunk_size);

    if (allocation.offset % m_chunk_size != 0)
        throw std::runtime_error("Allocation result is not aligned");

    auto context = co_await boost::asio::this_coro::context;
    for (auto i = 0ul; i < num_stripes; i++) {
        allocation_t alloc{.offset = allocation.offset + i * m_chunk_size,
                           .size = m_chunk_size};

        auto data_stripe = data.subspan(i * m_stripe_size,
                                        std::min(write_size, m_stripe_size));
        write_size -= m_stripe_size;

        auto encoded = m_rs.encode(data_stripe, m_chunk_size);
        std::vector<std::vector<std::size_t>> stripe_offsets(
            m_config.data_shards + m_config.parity_shards,
            std::vector<std::size_t>{0});

        // Translate offsets into chunk-local offsets for each stripe.
        const std::size_t stripe_offset = i * m_stripe_size;
        auto current_offset =
            std::lower_bound(offsets.begin(), offsets.end(), stripe_offset);

        for (size_t j = 0; j < m_config.data_shards; ++j) {
            const std::size_t chunk_offset = stripe_offset + j * m_chunk_size;
            while (current_offset != offsets.end() and
                   *current_offset < chunk_offset + m_chunk_size) {
                auto local_offset = *current_offset - chunk_offset;
                if (local_offset != 0) {
                    stripe_offsets.at(j).push_back(local_offset);
                }
                ++current_offset;
            }
        }

        co_await run_for_all<address, std::shared_ptr<storage_interface>>(
            m_ioc,
            [&](size_t i, auto storage) -> coro<address> {
                co_return co_await storage
                    ->write(alloc, encoded.get().at(i), stripe_offsets.at(i))
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

coro<std::size_t> ec_data_view::unlink(const address& addr) {
    auto storages = m_externals.get_storage_services();
    std::atomic<size_t> freed_bytes;
    co_await perform_for_address(
        m_ioc, addr,
        [this](uint128_t pointer) -> auto {
            return get_storage_pointer(pointer);
        },
        [&freed_bytes, this](size_t, std::shared_ptr<storage_interface> svc,
                             const address_info& info) -> coro<void> {
            auto freed_chunks = co_await svc->unlink(info.addr);
            // todo: recalculate parity data for deleted chunks/pages
            freed_bytes += freed_chunks.size() * m_chunk_size;
        },
        storages);

    // Now handle parity shards
    // Group fragments by stripe to process parity shards efficiently
    std::unordered_map<uint64_t, std::vector<fragment>> stripe_fragments;

    for (size_t i = 0; i < addr.size(); ++i) {
        auto frag = addr.get(i);
        auto [storage_id, storage_ptr] = get_storage_pointer(frag.pointer);

        // Calculate the stripe base pointer (aligned to stripe boundaries)
        uint64_t stripe_base = (storage_ptr / m_stripe_size) * m_stripe_size;
        stripe_fragments[stripe_base].push_back(frag);
    }

    // Process each affected stripe
    for (const auto& [stripe_base, fragments] : stripe_fragments) {
        // Create virtual parity addresses for each parity shard in this stripe
        for (size_t p = 0; p < m_config.parity_shards; ++p) {
            size_t parity_shard_id = m_config.data_shards + p;
            if (!storages[parity_shard_id])
                continue;

            address parity_addr;
            for (const auto& frag : fragments) {
                auto [data_shard_id, data_ptr] =
                    get_storage_pointer(frag.pointer);

                // Calculate the corresponding parity pointer
                uint64_t parity_ptr = stripe_base + (data_ptr % m_chunk_size);
                uint128_t global_parity_ptr =
                    get_global_pointer(parity_ptr, parity_shard_id);

                // Create a fragment for this parity piece
                fragment parity_frag{
                    .pointer = global_parity_ptr,
                    .size = std::min(frag.size,
                                     m_chunk_size - (data_ptr % m_chunk_size))};
                parity_addr.push(parity_frag);
            }

            // Unlink the parity fragments
            if (parity_addr.size() > 0) {
                co_await storages[parity_shard_id]->unlink(parity_addr);
            }
        }
    }

    co_return freed_bytes;
}

} // namespace uh::cluster::storage
