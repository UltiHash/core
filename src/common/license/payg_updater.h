#pragma once

#include "fetch.h"

#include <common/etcd/utils.h>
#include <common/types/common_types.h>

namespace uh::cluster::lic {

class payg_updater {
public:
    using callback_t = std::function<coro<std::string>()>;
    using exception_handler_t =
        std::function<backoff_action(const std::exception& e)>;

    payg_updater(
        boost::asio::io_context& ioc, etcd_manager& etcd,
        callback_t&& get_license,
        exception_handler_t&& exception_handler = [](const std::exception& e)
            -> backoff_action { return backoff_action::ABORT; })
        : m_ioc{ioc},
          m_etcd{etcd},
          m_get_license{std::forward<callback_t>(get_license)},
          m_exception_handler{
              std::forward<exception_handler_t>(exception_handler)} {}

    coro<void> update();
    coro<void> periodic_update(std::chrono::seconds interval);

private:
    boost::asio::io_context& m_ioc;
    etcd_manager& m_etcd;
    callback_t m_get_license;
    exception_handler_t m_exception_handler;
};

} // namespace uh::cluster::lic
