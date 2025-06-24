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
             std::size_t storage_id,
             const global_data_view_config& global_config)
        : m_executor{executor},
          m_etcd{etcd},
          m_group_config{config},
          m_global_config{global_config},
          m_subscriber(m_etcd, m_group_config.id, m_group_config.storages,
                       service_factory<storage_interface>(
                           m_executor.get_executor(),
                           m_global_config.storage_service_connection_count)) {
        boost::asio::co_spawn(
            executor.get_executor(),
            [this]() -> coro<void> { co_await repair(); },
            boost::asio::detached);
    }

private:
    executor& m_executor;
    etcd_manager& m_etcd;
    group_config m_group_config;
    global_data_view_config m_global_config;

    externals_subscriber m_subscriber;

    coro<void> repair() {

        auto timer = boost::asio::steady_timer(
            co_await boost::asio::this_coro::executor);
        timer.expires_after(1s);
        co_await timer.async_wait(boost::asio::use_awaitable);

        // if( error )
        // co_return;

        storage_assignment_triggers_manager::put(m_etcd, m_group_config.id,
                                                 true);
    }
};

} // namespace uh::cluster::storage
