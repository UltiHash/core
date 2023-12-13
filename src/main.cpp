//
// Created by masi on 7/17/23.
//

#include <system_error>
#include "common/cluster_config.h"
#include "storage/storage.h"
#include "deduplicatior/deduplicator.h"
#include "directory/directory.h"
#include "entrypoint/entrypoint.h"

#include <config.h>
#include "common/log.h"

using namespace uh::cluster;

void execute_role (const uh::cluster::role role, const std::size_t id) {

    switch (role) {
        case uh::cluster::DATASTORE_SERVICE: {
            LOG_INFO() << "starting data-store service";
            uh::cluster::storage ds(id);
            ds.run();
            break;
        }

        case uh::cluster::DEDUPLICATION_SERVICE: {
            LOG_INFO() << "starting deduplication service";
            uh::cluster::deduplicator dd (id);
            dd.run();
            break;
        }

        case uh::cluster::DIRECTORY_SERVICE: {
            LOG_INFO() << "starting directory service";
            uh::cluster::directory dr (id);
            dr.run();
            break;
        }

        case uh::cluster::ENTRYPOINT_SERVICE: {
            LOG_INFO() << "starting entrypoint service";
            uh::cluster::entrypoint en (id);
            en.run();
            break;
        }

    }

}

//void handleChange(etcd::Response response) {
//    LOG_INFO() << "action: " << response.action() << ", key: " << response.value().key() << ", value: " << response.value().as_string();
//    }

int main (int argc, char* args[]) {
    if (argc != 3) {
        throw std::invalid_argument("Usage: uh-cluster <role> <id>");
    }

    uh::log::config lc {
        .sinks = {
            uh::log::sink_config {
                .type = uh::log::sink_type::cout
            },
            uh::log::sink_config {
                .type = uh::log::sink_type::file,
                .filename = "log.log"
            }
        }
    };

    uh::log::init(lc);
    LOG_INFO() << "starting " << PROJECT_NAME << " " << PROJECT_VERSION << " on host " << boost::asio::ip::host_name();


    /*
    etcd::Client etcd("http://127.0.0.1:2379");
    std::shared_ptr<etcd::KeepAlive> keepalive = etcd.leasekeepalive(etcd_ttl).get();
    etcd.set("/uh/ds/0", boost::asio::ip::host_name(),keepalive->Lease());
    etcd::Watcher watcher("http://127.0.0.1:2379", "/uh", handleChange, true);

    sleep(1);
    keepalive->Cancel();
    sleep(10);
     */


    const auto role_str = std::string(args[1]);   // en, dd, dr, dn
    const std::size_t id = std::stoul(args[2]);
    const auto role = get_role (role_str);

    execute_role (role, id);
}
