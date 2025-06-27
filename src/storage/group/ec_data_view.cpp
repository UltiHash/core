#include "ec_data_view.h"

#include <common/coroutines/coro_util.h>
#include <common/telemetry/log.h>
#include <common/utils/integral.h>
#include <common/utils/strings.h>
#include <unordered_set>

namespace uh::cluster::storage {
ec_data_view::ec_data_view(boost::asio::io_context& ioc, etcd_manager& etcd,
                           std::size_t group_id, group_config config,
                           std::size_t service_connections)
    : m_ioc(ioc),
      m_config{config},
      m_stripe_size{m_config.get_stripe_size()},
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
              time_settings::instance().group_state_wait_timeout);
    LOG_DEBUG() << "[ec_data_view] group state is ready for group " << group_id;
}

std::vector<std::vector<std::size_t>>
ec_data_view::prepare_stripe_offsets(const std::vector<std::size_t>& offsets,
                                     std::size_t stripe_index,
                                     std::size_t stripe_data_size) const {
    auto base_offset = stripe_index * m_chunk_size;
    auto stripe_offsets = std::vector<std::vector<std::size_t>>(
        m_config.data_shards + m_config.parity_shards);

    // translate offsets into chunk-local offsets for each stripe.
    const std::size_t stripe_offset = stripe_index * m_stripe_size;
    auto current_offset =
        std::lower_bound(offsets.begin(), offsets.end(), stripe_offset);

    for (size_t j = 0; j < m_config.data_shards; ++j) {
        const std::size_t chunk_offset = stripe_offset + j * m_chunk_size;
        if (stripe_data_size > j * m_chunk_size) {
            // if chunk actually contains user data, and if so add a zero
            // offset to denote the start of the chunk and thus a new
            // fragment, as fragments may not span across chunks
            stripe_offsets[j].push_back(0 + base_offset);
        }
        while (current_offset != offsets.end() &&
               *current_offset < chunk_offset + m_chunk_size) {
            auto local_offset = *current_offset - chunk_offset;
            if (local_offset != 0) {
                stripe_offsets[j].push_back(local_offset + base_offset);
            }
            ++current_offset;
        }
    }

    std::size_t num_fragments = std::accumulate(
        stripe_offsets.begin(), stripe_offsets.end(), 0ul,
        [](std::size_t acc, const std::vector<std::size_t>& chunk_offsets) {
            return acc + chunk_offsets.size();
        });

    // goal: stripe_offsets[p].size() for any p == num_fragments
    for (std::size_t p = m_config.data_shards;
         p < m_config.data_shards + m_config.parity_shards; ++p) {
        for (std::size_t o = 0; o < num_fragments; ++o) {
            stripe_offsets[p].push_back(0 + base_offset);
        }
    }
    return stripe_offsets;
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
    LOG_DEBUG() << "storage services: " << ss.str();

    if (leader < 0 or leader >= (candidate_observer::id_t)m_config.storages)
        throw std::runtime_error("Invalid leader id: " +
                                 std::to_string(leader));

    auto num_stripes = div_ceil(data.size(), m_stripe_size);
    auto allocation = co_await storages.at(leader)->allocate(
        num_stripes * m_chunk_size, m_chunk_size);

    if (allocation.offset % m_chunk_size != 0)
        throw std::runtime_error("Allocation result is not aligned");

    auto parity_buffer = unique_buffer<char>(m_config.parity_shards *
                                             m_chunk_size * num_stripes);
    auto parities =
        split_buffer<char>(parity_buffer, m_chunk_size * num_stripes);

    auto storage_buffers_view =
        std::vector<std::vector<std::span<const char>>>();
    storage_buffers_view.reserve(m_config.storages);
    for (size_t i = 0; i < m_config.storages; ++i) {
        storage_buffers_view.emplace_back();
        storage_buffers_view.back().reserve(num_stripes);
    }
    // NOTE: buffer_views's data element will be pushed in the loop below
    for (size_t i = 0; i < m_config.parity_shards; ++i) {
        storage_buffers_view[m_config.data_shards + i].push_back(parities[i]);
    }

    std::vector<std::vector<std::size_t>> storage_offsets;
    storage_offsets.reserve(m_config.storages);
    for (size_t i = 0; i < m_config.storages; ++i) {
        storage_offsets.emplace_back();
    }

    std::optional<unique_buffer<>> last_stripe;

    for (auto i = 0ul; i < num_stripes; i++) {

        auto [data_view, data_view_size] = [&]() {
            auto data_view_size = m_stripe_size;
            auto sub_data = data.subspan(i * m_stripe_size);
            if (i == num_stripes - 1 && sub_data.size() < m_stripe_size) {
                data_view_size = sub_data.size();
                last_stripe.emplace(m_stripe_size);
                std::copy(sub_data.begin(), sub_data.end(),
                          last_stripe->span().begin());
                std::ranges::fill(last_stripe->span().subspan(sub_data.size()),
                                  0);
                auto d = std::span<const char>(last_stripe->string_view());
                return std::make_pair(split_buffer<const char>(d, m_chunk_size),
                                      data_view_size);
            } else {
                auto d = sub_data.first(m_stripe_size);
                return std::make_pair(split_buffer<const char>(d, m_chunk_size),
                                      data_view_size);
            }
        }();

        for (auto j = 0ul; j < m_config.data_shards; ++j) {
            storage_buffers_view[j].push_back(data_view[j]);
        }

        auto parity_view = std::vector<std::span<char>>();
        parity_view.reserve(m_config.parity_shards);
        for (const auto& p : parities) {
            parity_view.emplace_back(p.begin() + i * m_chunk_size,
                                     m_chunk_size);
        }

        m_rs.encode(data_view, parity_view);

        auto stripe_offsets =
            prepare_stripe_offsets(offsets, i, data_view_size);
        for (size_t j = 0; j < m_config.storages; ++j) {
            storage_offsets[j].insert(storage_offsets[j].end(),
                                      stripe_offsets[j].begin(),
                                      stripe_offsets[j].end());
        }
    }

    auto addresses =
        co_await run_for_all<address, std::shared_ptr<storage_interface>>(
            m_ioc,
            [&](size_t i, auto storage) -> coro<address> {
                auto storage_addr = co_await storage->write(
                    allocation, storage_buffers_view[i], storage_offsets[i]);
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

    address rv;
    std::size_t base_offset = allocation.offset * m_config.data_shards;
    for (auto it = offsets.begin(); it != offsets.end(); it++) {
        auto next = std::next(it);
        std::size_t frag_size =
            next == offsets.end() ? data.size_bytes() - *it : *next - *it;
        rv.emplace_back(base_offset + *it, frag_size);
    }
    // WARNING: this is a local address and won't work with multiple storage
    // groups

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
                LOG_DEBUG() << std::format(
                    "[read_from_storages: group {}, storage {}] try to read...",
                    m_config.id, id);

                co_await storage->read_address(info.addr, buffer,
                                               info.pointer_offsets);
                LOG_DEBUG() << std::format(
                    "[read_from_storages: group {}, storage {}] done",
                    m_config.id, id);
                co_return true;
            } catch (const std::exception& e) {
                LOG_DEBUG() << std::format("[read_from_storages: group {}, "
                                           "storage {}] failed, ",
                                           m_config.id, id)
                            << e.what();
                co_return false;
            } catch (...) {
                LOG_DEBUG()
                    << std::format("[read_from_storages: group {}, storage {}] "
                                   "failed, unknown exception",
                                   m_config.id, id);
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
                    } catch (const std::exception& e) {
                        LOG_ERROR() << "Failed to read from storage " << id
                                    << ": " << e.what();
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
                co_return co_await storage->get_used_space();
            },
            storages);

    auto used_space =
        std::accumulate(used_spaces.begin(), used_spaces.end(), 0ul);

    co_return used_space;
}

[[nodiscard]] coro<address> ec_data_view::link(const address& addr) {
    auto aligned_addr = address{};
    for (auto& frag : addr.fragments) {
        auto a = split_fragment(frag.pointer, frag.size);
        aligned_addr.append(a);
    }

    auto storages = m_externals.get_storage_services();
    auto addr_map = co_await perform_for_address<address>(
        m_ioc, aligned_addr,
        [this](uint128_t pointer) -> auto {
            return get_storage_pointer(pointer);
        },
        [](std::shared_ptr<storage_interface> svc, const address_info& info)
            -> coro<address> { co_return co_await svc->link(info.addr); },
        storages);

    address parity_addr = compute_parity_address(aligned_addr);

    for (size_t p = 0; p < m_config.parity_shards; ++p) {
        size_t parity_shard_id = m_config.data_shards + p;
        if (!storages[parity_shard_id])
            continue;
        auto rejected_addr =
            co_await storages[parity_shard_id]->link(parity_addr);
        if (!rejected_addr.empty()) {
            throw std::runtime_error("Failed to link parity address: " +
                                     rejected_addr.to_string());
        }
    }

    address rv;
    for (const auto& a : addr_map | std::views::values) {
        rv.append(a);
    }

    co_return rv;
}

coro<std::size_t> ec_data_view::unlink(const address& addr) {
    auto aligned_addr = address{};
    for (auto& frag : addr.fragments) {
        auto a = split_fragment(frag.pointer, frag.size);
        aligned_addr.append(a);
    }

    auto storages = m_externals.get_storage_services();
    auto freed_pages_map = co_await perform_for_address<std::size_t>(
        m_ioc, aligned_addr,
        [this](uint128_t pointer) -> auto {
            return get_storage_pointer(pointer);
        },
        [](std::shared_ptr<storage_interface> svc, const address_info& info)
            -> coro<std::size_t> { co_return co_await svc->unlink(info.addr); },
        storages);

    address parity_addr = compute_parity_address(aligned_addr);

    for (size_t p = 0; p < m_config.parity_shards; ++p) {
        size_t parity_shard_id = m_config.data_shards + p;
        if (!storages[parity_shard_id])
            continue;
        co_await storages[parity_shard_id]->unlink(parity_addr);
    }

    auto freed_bytes = std::accumulate(
        std::ranges::begin(freed_pages_map | std::views::values),
        std::ranges::end(freed_pages_map | std::views::values), 0ul,
        [](std::size_t acc, std::size_t val) { return acc + val; });
    co_return freed_bytes;
}

address ec_data_view::compute_parity_address(const address& addr) {
    address parity_addr;
    for (size_t i = 0; i < addr.size(); ++i) {
        auto frag = addr.get(i);
        std::size_t chunk_base = (frag.pointer / m_stripe_size) * m_chunk_size;
        parity_addr.push({chunk_base, m_chunk_size});
    }
    return parity_addr;
}

} // namespace uh::cluster::storage
