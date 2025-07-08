#pragma once

#include <common/coroutines/coro_util.h>
#include <common/ec/reedsolomon_c.h>
#include <storage/global/config.h>
#include <storage/group/config.h>
#include <storage/group/externals.h>
#include <storage/group/internals.h>
#include <storage/interfaces/local_storage.h>

namespace uh::cluster::storage {

class storages_reader {
public:
    using callback_t = reader::callback_t;
    storages_reader(etcd_manager& etcd, std::size_t group_id,
                    std::size_t num_storages, //
                    std::size_t my_storage_id,
                    std::shared_ptr<storage_interface> my_storage,
                    service_factory<storage_interface> service_factory,
                    callback_t callback = nullptr)
        : m_key{get_prefix(group_id).storage_hostports},
          m_my_storage_id{my_storage_id},
          m_my_storage{my_storage},
          m_storage_index{num_storages},
          m_storage_hostports{m_key,
                              std::move(service_factory),
                              {m_storage_index},
                              my_storage_id},
          m_reader{"hostports_reader",
                   etcd,
                   m_key,
                   {m_storage_hostports},
                   std::move(callback)} {}

    ~storages_reader() { LOG_DEBUG() << "hostports_reader is destroyed"; }

    // NOTE: get method is heavy: it retrieves all atomic variables
    auto get_storage_services() {
        auto storages = m_storage_index.get();
        storages[m_my_storage_id] = m_my_storage;
        return storages;
    };

private:
    std::string m_key;

    std::size_t m_my_storage_id;
    std::shared_ptr<storage_interface> m_my_storage;

    storage_index m_storage_index;
    hostports_observer<storage_interface> m_storage_hostports;

    reader m_reader;
};

class repairer {
public:
    repairer(boost::asio::io_context& ioc, etcd_manager& etcd,
             const group_config& config, std::size_t repairing_size,
             std::vector<std::shared_ptr<storage_interface>> storages,
             std::vector<storage_state> storage_states,
             const global_data_view_config& global_config)
        : m_ioc{ioc},
          m_etcd{etcd},
          m_config{config},
          m_global_config{global_config},
          m_repairing_size{repairing_size},

          m_storages{std::move(storages)},
          m_storage_states{std::move(storage_states)} {

        LOG_DEBUG() << "Repairer created for group " << m_config.id;
        boost::asio::co_spawn(
            ioc, [this]() -> coro<void> { co_await repair(); },
            boost::asio::detached);
    }

    ~repairer() {
        LOG_DEBUG() << "Repairer destroyed for group " << m_config.id;
    }

private:
    boost::asio::io_context& m_ioc;
    etcd_manager& m_etcd;
    group_config m_config;
    global_data_view_config m_global_config;
    std::size_t m_repairing_size;

    std::vector<std::shared_ptr<storage_interface>> m_storages;
    std::vector<storage_state> m_storage_states;

    coro<void> repair() {
        LOG_DEBUG() << "Repairing started for group " << m_config.id;

        try {
            std::size_t m_chunk_size = m_config.get_stripe_unit_size();
            std::size_t num_stripes = m_repairing_size / m_chunk_size;
            if (m_repairing_size % m_chunk_size != 0) {
                LOG_ERROR()
                    << "Repairing size must be a multiple of chunk size";
                co_return;
            }

            auto m_rs = reedsolomon_c{m_config.data_shards,
                                      m_config.parity_shards, m_chunk_size};

            unique_buffer<char> stripe(m_chunk_size * m_config.storages);
            std::vector<std::span<char>> shards;
            shards.reserve(m_config.storages);
            for (auto i = 0ul; i < m_config.storages; ++i) {
                auto shard =
                    stripe.span().subspan(m_chunk_size * i, m_chunk_size);
                shards.push_back(shard);
            }
            for (auto i = 0ul; i < num_stripes; ++i) {
                LOG_DEBUG() << "start repairing data for stripe " << i;
                (void)m_chunk_size;
                address addr;
                addr.emplace_back(i * m_chunk_size, m_chunk_size);
                {
                    auto v_succeeded = co_await run_for_all<
                        bool, std::shared_ptr<storage_interface>>(
                        m_ioc,
                        [&](std::size_t id,
                            const std::shared_ptr<storage_interface>& storage)
                            -> coro<bool> {
                            try {
                                if (m_storage_states[id] ==
                                    storage_state::NEW) {
                                    LOG_DEBUG() << "Storage " << id
                                                << " is not assigned, skipping";
                                    co_return true;
                                }
                                LOG_DEBUG() << "read from storage " << id;
                                co_await storage->read_address(addr, shards[id],
                                                               {0});
                                LOG_DEBUG()
                                    << "read from storage " << id << " done";
                                co_return true;
                            } catch (const std::exception& e) {
                                LOG_ERROR() << "Failed to read from storage "
                                            << id << ": " << e.what();
                                co_return false;
                            }
                        },
                        m_storages);

                    auto failed = std::ranges::any_of(
                        v_succeeded, [](auto s) { return s == false; });
                    if (failed) {
                        LOG_ERROR() << "Failed to read stripe " << i
                                    << " from storages while repairing group "
                                    << m_config.id;
                        co_return;
                    }
                }

                std::vector<data_stat> stats;
                stats.reserve(m_config.storages);
                for (const auto& f : m_storage_states) {
                    if (f == storage_state::ASSIGNED) {
                        stats.push_back(data_stat::valid);
                    } else {
                        stats.push_back(data_stat::lost);
                    }
                }

                LOG_DEBUG() << "[read_address] call recover";
                m_rs.recover(shards, stats);

                LOG_DEBUG() << "recovering reference count data";
                auto stripe_refcounts =
                    co_await run_for_all<std::size_t,
                                         std::shared_ptr<storage_interface>>(
                        m_ioc,
                        [&](std::size_t id,
                            const std::shared_ptr<storage_interface>& storage)
                            -> coro<std::size_t> {
                            if (m_storage_states[id] == storage_state::NEW) {
                                co_return 0;
                            }
                            auto refcounts =
                                co_await storage->get_refcounts({i});
                            co_return refcounts.front().count;
                        },
                        m_storages);

                refcount_t max_stripe_refcount = {
                    .stripe_id = i,
                    .count = std::accumulate(
                        stripe_refcounts.begin(), stripe_refcounts.end(), 0ull,
                        [](std::size_t acc, std::size_t count) {
                            return std::max(acc, count);
                        })};

                {
                    auto alloc = allocation_t{i * m_chunk_size, m_chunk_size};
                    auto v_succeeded = co_await run_for_all<
                        bool, std::shared_ptr<storage_interface>>(
                        m_ioc,
                        [&](std::size_t id,
                            const std::shared_ptr<storage_interface>& storage)
                            -> coro<bool> {
                            try {
                                if (m_storage_states[id] !=
                                    storage_state::NEW) {
                                    LOG_DEBUG() << "Storage " << id
                                                << " is not new, skipping";
                                    co_return true;
                                }
                                LOG_DEBUG() << "write to storage " << id;
                                co_await storage->write(alloc, {shards[id]},
                                                        {max_stripe_refcount});
                                LOG_DEBUG()
                                    << "write to storage " << id << " done";
                                co_return true;
                            } catch (const std::exception& e) {
                                LOG_ERROR() << "Failed to write to storage "
                                            << id << ": " << e.what();
                                co_return false;
                            }
                        },
                        m_storages);

                    auto failed = std::ranges::any_of(
                        v_succeeded, [](auto s) { return s == false; });
                    if (failed) {
                        LOG_ERROR() << "Failed to write stripe " << i
                                    << " from storages while repairing group "
                                    << m_config.id;
                        co_return;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cout << "exception thrown: " << e.what() << std::endl;
            co_return;
        }

        std::cout << "Trigger assignment" << std::endl;
        storage_assignment_triggers_manager::put(m_etcd, m_config.id, true);
        std::cout << "Repairing finished" << std::endl;
    }
};

} // namespace uh::cluster::storage
