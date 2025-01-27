#pragma once

#include <common/etcd/utils.h>
#include <common/types/common_types.h>

namespace uh::cluster {

class payg_updater {
public:
    using callback_t = std::function<coro<std::string>()>;

    payg_updater(boost::asio::io_context& ioc, etcd_manager& etcd,
                 callback_t&& get_license)
        : m_ioc{ioc},
          m_etcd{etcd},
          m_get_license{std::forward<callback_t>(get_license)} {}

    coro<void> update();
    coro<void> periodic_update(std::chrono::seconds interval);

private:
    boost::asio::io_context& m_ioc;
    etcd_manager& m_etcd;
    callback_t m_get_license;
};

} // namespace uh::cluster
