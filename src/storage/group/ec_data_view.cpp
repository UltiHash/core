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
          service_factory<storage_interface>(ioc, service_connections)) {

    LOG_DEBUG() << "[ec_data_view] waiting group state for group " << group_id;
    etcd.wait(ns::root.storage_groups[group_id].group_state,
              GROUP_STATE_WAIT_TIMEOUT);
    LOG_DEBUG() << "[ec_data_view] group state is ready for group " << group_id;
}

coro<address> ec_data_view::write(std::span<const char> data,
                                  const std::vector<std::size_t>& offsets) {

    if (*m_externals.get_group_state() != group_state::HEALTHY)
        throw std::runtime_error("group state should be healthy: " +
                                 serialize(*m_externals.get_group_state()));

    // NOTE: We intentionally don't check the storage's state here, as it is
    // essential for supporting repairs.

    auto storages = m_externals.get_storage_services();
    auto leader = *m_externals.get_leader();

    LOG_DEBUG() << "[ec_data_view] writing data to leader " << leader;
    std::stringstream ss;
    for (auto& s : storages) {
        ss << std::hex << serialize(s) << ", ";
    }
    LOG_DEBUG() << "storage states: " << ss.str();

    if (leader < 0 or leader >= (candidate_observer::id_t)m_config.storages)
        throw std::runtime_error("Invalid leader id: " +
                                 std::to_string(leader));

    auto write_size = data.size();
    auto num_stripes = div_ceil(write_size, m_stripe_size);
    auto allocation = co_await storages.at(leader)->allocate(
        num_stripes * m_chunk_size, m_chunk_size);

    if (allocation.offset % m_chunk_size != 0)
        throw std::runtime_error("Allocation result is not aligned");

    auto context = co_await boost::asio::this_coro::context;

    // NOTE: Now we allocatate a buffer for data shards, not for whole stripe.
    // It's because we allocate parity shards in the encode function. I'd like
    // to change this in the future.
    unique_buffer<char> stripe(m_chunk_size * m_config.data_shards);
    address rv;
    std::size_t user_data_size = data.size_bytes();

    for (auto i = 0ul; i < num_stripes; i++) {
        allocation_t alloc{.offset = allocation.offset + i * m_chunk_size,
                           .size = m_chunk_size};
        auto stripe_data = data.subspan(i * m_stripe_size);
        auto stripe_data_size = std::min(stripe_data.size(), m_stripe_size);
        if (stripe_data_size != m_stripe_size) {
            std::copy(stripe_data.begin(), stripe_data.end(),
                      stripe.span().begin());
            std::ranges::fill(stripe.span().subspan(stripe_data_size), 0);
            stripe_data = stripe.span();
        } else {
            stripe_data = stripe_data.first(stripe_data_size);
        }

        write_size -= stripe_data_size;

        auto encoded = m_rs.encode(stripe_data);

        std::vector<std::vector<std::size_t>> stripe_offsets(
            m_config.data_shards + m_config.parity_shards);

        // translate offsets into chunk-local offsets for each stripe.
        const std::size_t stripe_offset = i * m_stripe_size;
        auto current_offset =
            std::lower_bound(offsets.begin(), offsets.end(), stripe_offset);

        for (size_t j = 0; j < m_config.data_shards; ++j) {
            const std::size_t chunk_offset = stripe_offset + j * m_chunk_size;
            if (stripe_data_size > j * m_chunk_size) {
                // if chunk actually contains user data, and if so add a zero
                // offset to denote the start of the chunk and thus a new
                // fragment, as fragments may not span across chunks
                stripe_offsets.at(j).push_back(0);
            }
            while (current_offset != offsets.end() and
                   *current_offset < chunk_offset + m_chunk_size) {
                auto local_offset = *current_offset - chunk_offset;
                if (local_offset != 0) {
                    stripe_offsets.at(j).push_back(local_offset);
                }
                ++current_offset;
            }
        }

        std::size_t num_fragments = std::accumulate(
            stripe_offsets.begin(), stripe_offsets.end(), 0ul,
            [](std::size_t acc, std::vector<std::size_t> chunk_offsets) {
                return acc + chunk_offsets.size();
            });

        // goal: stripe_offsets.at(p).size() for any p == num_fragments
        for (std::size_t p = m_config.data_shards;
             p < m_config.data_shards + m_config.parity_shards; ++p) {
            for (std::size_t o = 0; o < num_fragments; ++o) {
                stripe_offsets.at(p).push_back(0);
            }
        }

        auto addresses =
            co_await run_for_all<address, std::shared_ptr<storage_interface>>(
                m_ioc,
                [&](size_t i, auto storage) -> coro<address> {
                    auto storage_addr = co_await storage
                                            ->write(alloc, encoded.get().at(i),
                                                    stripe_offsets.at(i))
                                            .continue_trace(context);
                    address global_addr;
                    // translate storage address into global address
                    for (std::size_t j = 0; j < storage_addr.size(); ++j) {
                        fragment frag = storage_addr.get(j);
                        global_addr.emplace_back(
                            get_global_pointer(frag.pointer, i), frag.size);
                    }
                    co_return global_addr;
                },
                storages);

        // combine partial addresses into complete return address
        for (std::size_t j = 0; j < m_config.data_shards; ++j) {
            for (auto& frag : addresses.at(j).fragments) {
                if (user_data_size == 0) {
                    break;
                }

                if (frag.size < user_data_size) {
                    user_data_size -= frag.size;
                    rv.emplace_back(frag.pointer, frag.size);
                } else {
                    rv.emplace_back(frag.pointer, user_data_size);
                    user_data_size = 0;
                }
            }
        }
    }
    co_return rv;
}

address ec_data_view::split_fragment(const uint128_t& pointer,
                                     std::size_t read_size) const {
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
    return addr;
}

coro<shared_buffer<>> ec_data_view::read(const uint128_t& pointer,
                                         std::size_t read_size) {
    auto rv = shared_buffer<>(read_size);
    auto addr = split_fragment(pointer, read_size);
    co_await read_address(addr, rv);

    co_return rv;
}

coro<std::unordered_map<std::size_t, bool>> ec_data_view::read_from_storages(
    std::unordered_map<std::size_t, address_info> addr_info_map,
    std::span<char> buffer) {
    co_return co_await run_for_all<bool>(
        m_ioc,
        // NOTE: doesn't check storage_states, since unassigned storages will
        // throw exception
        [this, buffer](std::size_t id, const address_info& info) -> coro<bool> {
            try {
                auto storage = m_externals.get_storage_service(id);
                auto state = m_externals.get_storage_states().at(id);
                if (storage == nullptr or *state != storage_state::ASSIGNED) {
                    co_return false;
                }
                LOG_DEBUG() << "try to read from storage " << id;
                co_await storage->read_address(info.addr, buffer,
                                               info.pointer_offsets);
                LOG_DEBUG() << "read from storage done" << id;
                co_return true;
            } catch (const std::exception& e) {
                LOG_ERROR() << "Failed to read address: " << e.what();
                co_return false;
            } catch (...) {
                LOG_ERROR() << "Failed to read address: Unknown exception";
                co_return false;
            }
        },
        addr_info_map);
}

std::unordered_map<uint64_t, std::vector<std::pair<fragment, std::size_t>>>
ec_data_view::get_stripe_ids(
    std::unordered_map<std::size_t, address_info> addr_info_map,
    std::unordered_map<std::size_t, bool> success_map) {
    std::unordered_map<uint64_t, std::vector<std::pair<fragment, std::size_t>>>
        rv;
    for (auto& [id, success] : success_map) {
        if (not success) {
            auto& addr = addr_info_map[id].addr;
            for (size_t i = 0; i < addr.fragments.size(); ++i) {
                auto& frag = addr.fragments[i];
                auto global_pointer = get_global_pointer(frag.pointer, id);
                auto stripe_id = static_cast<uint64_t>(div_floor(
                    global_pointer, static_cast<uint128_t>(m_stripe_size)));

                rv[stripe_id].emplace_back(
                    fragment(global_pointer, frag.size),
                    addr_info_map[id].pointer_offsets[i]);
            }
        }
    }
    return rv;
}

coro<std::size_t> ec_data_view::read_address(const address& addr,
                                             std::span<char> buffer) {
    auto aligned_addr = address{};
    for (auto& frag : addr.fragments) {
        auto a = split_fragment(frag.pointer, frag.size);
        aligned_addr.append(a);
    }
    auto addr_info_map = extract_node_address_map(
        aligned_addr, [this](uint128_t pointer) -> auto {
            return get_storage_pointer(pointer);
        });

    for (auto& id : addr_info_map | std::views::keys) {
        if (id >= m_config.data_shards) {
            throw std::runtime_error("Invalid storage id in address: " +
                                     std::to_string(id));
        }
    }
    auto success_map = co_await read_from_storages(addr_info_map, buffer);

    auto success = std::ranges::all_of(success_map | std::views::values,
                                       [](auto success) { return success; });

    if (success)
        co_return addr.data_size();

    LOG_DEBUG() << "[read_address] try to reconstruct";
    auto storages = m_externals.get_storage_services();
    auto states = m_externals.get_storage_states();
    for (auto i = 0ul; i < states.size(); ++i) {
        LOG_DEBUG() << "[read_address] storage " << i
                    << " instance: " << storages[i]
                    << " state: " << serialize(*states[i]);
        if (storages[i] != nullptr && *states[i] != storage_state::ASSIGNED)
            storages[i] = nullptr;
    }

    for (auto& [id, success] : success_map) {
        if (not success) {
            LOG_DEBUG() << "[read_address] storage " << id
                        << " is not successful, set to nullptr";
            storages[id] = nullptr;
        }
    }

    auto num_valid_storages =
        std::ranges::count_if(storages, [](auto& s) { return s != nullptr; });

    LOG_DEBUG() << "[read_address] num of valid storages: "
                << num_valid_storages;

    if ((std::size_t)num_valid_storages < m_config.data_shards) {
        throw std::runtime_error("Failed to read address: there's not enough "
                                 "valid storages");
    }

    // NOTE: this function translates storage pointer to global pointer
    auto stripe_map = get_stripe_ids(addr_info_map, success_map);

    unique_buffer<char> stripe(m_chunk_size * m_config.storages);
    std::vector<std::span<char>> shards;
    shards.reserve(m_config.storages);
    for (auto i = 0ul; i < m_config.storages; ++i) {
        auto shard = stripe.span().subspan(m_chunk_size * i, m_chunk_size);
        shards.push_back(shard);
    }

    for (auto& [stripe_id, frags_and_offsets] : stripe_map) {
        address _addr;
        _addr.emplace_back(m_chunk_size * stripe_id, m_chunk_size);

        LOG_DEBUG() << "[read_address] call run_for_all for a stripe "
                    << stripe_id;
        auto succeeded =
            co_await run_for_all<bool, std::shared_ptr<storage_interface>>(
                m_ioc,
                [&shards,
                 &_addr](std::size_t id,
                         const std::shared_ptr<storage_interface>& storage)
                    -> coro<bool> {
                    try {
                        if (storage == nullptr) {
                            std::ranges::fill(shards[id], 0);
                            co_return false;
                        } else {
                            LOG_DEBUG() << "read from storage " << id;
                            co_await storage->read_address(_addr, shards[id],
                                                           {0});
                            LOG_DEBUG()
                                << "read from storage " << id << " done";
                            co_return true;
                        }
                    } catch (...) {
                        LOG_ERROR() << "Failed to read from storage " << id;
                        std::ranges::fill(shards[id], 0);
                        co_return false;
                    }
                },
                storages);

        auto num_valid_storages =
            std::ranges::count_if(succeeded, [](auto s) { return s == true; });

        if ((std::size_t)num_valid_storages < m_config.data_shards) {
            throw std::runtime_error(
                "Failed to read address: there's not enough valid shards");
        }

        std::vector<data_stat> stats;
        stats.reserve(m_config.storages);
        for (const auto& f : succeeded) {
            if (f) {
                stats.push_back(data_stat::valid);
            } else {
                stats.push_back(data_stat::lost);
            }
        }

        LOG_DEBUG() << "[read_address] call recover";
        m_rs.recover(shards, stats);

        LOG_DEBUG() << "[read_address] copy recovered data to buffer";
        for (auto& [frag, offset] : frags_and_offsets) {
            auto [id, pointer] = get_storage_pointer(frag.pointer);
            std::size_t chunk_offset = pointer % m_chunk_size;

            std::ranges::copy(shards[id].subspan(chunk_offset, frag.size),
                              buffer.subspan(offset, frag.size).begin());
        }
    }

    co_return addr.data_size();
}

coro<std::size_t> ec_data_view::get_used_space() {
    auto storages = m_externals.get_storage_services();
    auto context = co_await boost::asio::this_coro::context;
    auto used_spaces =
        co_await run_for_all<std::size_t, std::shared_ptr<storage_interface>>(
            m_ioc,
            [&](size_t i, auto storage) -> coro<std::size_t> {
                if (i >= m_config.data_shards) {
                    co_return 0; // skip parity shards
                }
                co_return co_await storage->get_used_space().continue_trace(
                    context);
            },
            storages);

    auto used_space =
        std::accumulate(used_spaces.begin(), used_spaces.end(), 0ul);

    co_return used_space;
}

[[nodiscard]] coro<address> ec_data_view::link(const address& addr) {
    co_return address{};
}

coro<std::size_t> ec_data_view::unlink(const address& addr) { co_return 0; }

} // namespace uh::cluster::storage
