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
      m_chunk_size{m_config.get_stripe_unit_size()},
      m_rs{config.data_shards, config.parity_shards, m_chunk_size},
      m_externals(
          etcd, group_id, config.storages,
          service_factory<storage_interface>(ioc, service_connections)) {

    LOG_DEBUG() << "[ec_data_view] waiting group state for group " << group_id;
    etcd.wait(ns::root.storage_groups[group_id].group_state,
              time_settings::instance().group_state_wait_timeout);
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

    std::vector<std::size_t> stripe_refcounts;
    stripe_refcounts.reserve(num_stripes);

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
    }

    address addr = compute_address(offsets, data.size_bytes(), allocation);
    // WARNING: this is a group address and won't work with multiple storage
    // groups
    auto refcounts = extract_refcounts(addr);

    co_await run_for_all<void, std::shared_ptr<storage_interface>>(
        m_ioc,
        [&](size_t i, auto storage) -> coro<void> {
            co_await storage->write(allocation, storage_buffers_view[i],
                                    refcounts);
        },
        storages);

    co_return addr;
}
address ec_data_view::compute_address(const std::vector<std::size_t>& offsets,
                                      const std::size_t data_size,
                                      const allocation_t& allocation) const {
    address rv;
    std::size_t base_offset = allocation.offset * m_config.data_shards;
    for (auto it = offsets.begin(); it != offsets.end(); it++) {
        auto next = std::next(it);
        std::size_t frag_size =
            next == offsets.end() ? data_size - *it : *next - *it;
        rv.emplace_back(base_offset + *it, frag_size);
    }
    return rv;
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
    address aligned_addr = get_aligned_address(addr);
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
address ec_data_view::get_aligned_address(const address& addr) const {
    address aligned_addr;
    for (auto& frag : addr.fragments) {
        auto a = split_fragment(frag.pointer, frag.size);
        aligned_addr.append(a);
    }
    return aligned_addr;
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

std::vector<refcount_t>
ec_data_view::extract_refcounts(const address& addr) const {
    std::map<std::size_t, std::size_t> refcount_by_stripe;

    for (const auto& frag : addr.fragments) {
        auto group_pointer = pointer_traits::get_group_pointer(frag.pointer);
        std::size_t first_stripe = group_pointer / m_stripe_size;
        std::size_t last_stripe =
            (group_pointer + frag.size - 1) / m_stripe_size;
        for (size_t stripe_id = first_stripe; stripe_id <= last_stripe;
             stripe_id++) {
            refcount_by_stripe[stripe_id]++;
        }
    }

    std::vector<refcount_t> refcounts;
    refcounts.reserve(refcount_by_stripe.size());
    for (const auto& [stripe_id, count] : refcount_by_stripe) {
        refcounts.emplace_back(stripe_id, count);
    }
    return refcounts;
}

address ec_data_view::compute_rejected_address(
    const std::vector<std::vector<refcount_t>>& rejected_refcounts,
    const address& original_addr) {
    std::unordered_set<std::size_t> rejected_stripes;
    for (const auto& refcount : rejected_refcounts) {
        for (const auto& rc : refcount) {
            rejected_stripes.insert(rc.stripe_id);
        }
    }

    if (rejected_stripes.empty()) {
        return {};
    }

    address rv;
    for (const auto& frag : original_addr.fragments) {
        auto group_pointer = pointer_traits::get_group_pointer(frag.pointer);
        std::size_t first_stripe = group_pointer / m_stripe_size;
        std::size_t last_stripe =
            (group_pointer + frag.size - 1) / m_stripe_size;
        bool has_overlap = false;
        for (size_t stripe_id = first_stripe; stripe_id <= last_stripe;
             stripe_id++) {
            if (rejected_stripes.contains(stripe_id)) {
                has_overlap = true;
            }
        }
        if (has_overlap) {
            rv.push(frag);
        }
    }

    return rv;
}

[[nodiscard]] coro<address> ec_data_view::link(const address& addr) {
    auto refcounts = extract_refcounts(addr);
    auto storages = m_externals.get_storage_services();
    auto link_rvs = co_await run_for_all<std::vector<refcount_t>,
                                         std::shared_ptr<storage_interface>>(
        m_ioc,
        [&](size_t i, auto storage) -> coro<std::vector<refcount_t>> {
            co_return co_await storage->link(refcounts);
        },
        storages);

    co_return compute_rejected_address(link_rvs, addr);
}

coro<std::size_t> ec_data_view::unlink(const address& addr) {
    auto refcounts = extract_refcounts(addr);
    auto storages = m_externals.get_storage_services();

    auto unlink_rvs =
        co_await run_for_all<std::size_t, std::shared_ptr<storage_interface>>(
            m_ioc,
            [&](size_t i, auto storage) -> coro<std::size_t> {
                std::size_t freed = co_await storage->unlink(refcounts);
                if (i >= m_config.data_shards) {
                    co_return 0;
                }
                co_return freed;
            },
            storages);

    auto freed_bytes =
        std::accumulate(unlink_rvs.begin(), unlink_rvs.end(), 0ul);
    co_return freed_bytes;
}

} // namespace uh::cluster::storage
