#pragma once

#include <common/execution/executor.h>
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
    repairer(executor& executor, etcd_manager& etcd, const group_config& config,
             std::shared_ptr<local_storage> my_storage,
             std::vector<std::shared_ptr<storage_interface>> storages,
             std::vector<storage_state> storage_states,
             const global_data_view_config& global_config)
        : m_etcd{etcd},
          m_group_config{config},
          m_global_config{global_config},

          m_storages{std::move(storages)},
          m_storage_states{std::move(storage_states)},

          m_promise{},
          m_future{m_promise.get_future()},
          m_signal{} {

        LOG_DEBUG() << "Repairer created for group " << m_group_config.id;
        boost::asio::co_spawn(
            executor.get_executor(),
            [this]() -> coro<void> { co_await repair(); },
            boost::asio::bind_cancellation_slot(m_signal.slot(),
                                                boost::asio::detached));
    }

    ~repairer() {
        LOG_DEBUG() << "Repairer destroyed for group " << m_group_config.id;
        m_signal.emit(boost::asio::cancellation_type::all);
        m_future.get();
    }

private:
    etcd_manager& m_etcd;
    group_config m_group_config;
    global_data_view_config m_global_config;

    std::vector<std::shared_ptr<storage_interface>> m_storages;
    std::vector<storage_state> m_storage_states;

    std::promise<void> m_promise;
    std::future<void> m_future;
    boost::asio::cancellation_signal m_signal;

    coro<void> repair() {
        LOG_DEBUG() << "Repairing started for group " << m_group_config.id;

        auto timer = boost::asio::steady_timer(
            co_await boost::asio::this_coro::executor);
        timer.expires_after(1s);
        try {
            co_await timer.async_wait(boost::asio::use_awaitable);

            // auto state = co_await boost::asio::this_coro::cancellation_state;
            // while (state.cancelled() == boost::asio::cancellation_type::none)
            // {
            //     // Do repairing for certain number of stripes
            // }
        } catch (const boost::system::system_error& e) {
            if (e.code() == boost::asio::error::operation_aborted) {
                std::cout << "Repairing cancelled" << std::endl;
                m_promise.set_value();
                co_return;
            } else {
                std::cout << "Repairing throws exception" << std::endl;
                throw;
            }
        }

        std::cout << "Trigger assignment" << std::endl;
        storage_assignment_triggers_manager::put(m_etcd, m_group_config.id,
                                                 true);
        std::cout << "Repairing finished" << std::endl;
        m_promise.set_value();
    }
};

} // namespace uh::cluster::storage
