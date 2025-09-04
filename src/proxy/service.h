#pragma once

#include <common/network/server.h>
#include <common/etcd/service.h>
#include <storage/global/data_view.h>
#include <proxy/cache/disk/manager.h>
#include "config.h"

#include <boost/asio.hpp>


namespace uh::cluster::proxy {

class service {
public:
    service(boost::asio::io_context& ioc, const service_config& sc, const config& c);

private:
    using manager = cache::disk::manager;

    boost::asio::io_context& m_ioc;
    etcd_manager m_etcd;
    std::unique_ptr<storage::data_view> m_dv;
    manager m_mgr;
    server m_server;
};

} // namespace uh::cluster::proxy
