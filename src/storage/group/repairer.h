#pragma once

#include <common/execution/executor.h>
#include <common/service_interfaces/storage_interface.h>
#include <storage/global/config.h>
#include <storage/group/config.h>
#include <storage/group/externals.h>
#include <storage/group/internals.h>

namespace uh::cluster::storage {

class repairer {
public:
    repairer(executor& executor, etcd_manager& etcd, const group_config& config,
             std::size_t my_storage_id,
             std::shared_ptr<storage_interface> my_storage,
             const global_data_view_config& global_config)
        : m_executor{executor},
          m_etcd{etcd},
          m_group_config{config},
          m_storage_id{my_storage_id},
          m_global_config{global_config},

          m_storages_reader(
              m_etcd, m_group_config.id, m_group_config.storages, //
              m_storage_id, my_storage,
              service_factory<storage_interface>(
                  m_executor.get_executor(),
                  m_global_config.storage_service_connection_count)),

          m_promise{},
          m_future{m_promise.get_future()},
          m_signal{} {

        (void)m_executor;
        (void)m_etcd;
        (void)m_group_config;
        LOG_DEBUG() << "Repairer created for group " << m_group_config.id
                    << " storage " << my_storage_id;
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
    executor& m_executor;
    etcd_manager& m_etcd;
    group_config m_group_config;
    std::size_t m_storage_id;
    global_data_view_config m_global_config;

    storages_reader m_storages_reader;

    std::promise<void> m_promise;
    std::future<void> m_future;
    boost::asio::cancellation_signal m_signal;

    coro<void> repair() {
        LOG_DEBUG() << "Repairing started for group " << m_group_config.id;

        auto storages = m_storages_reader.get_storage_services();
        (void)storages;

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
