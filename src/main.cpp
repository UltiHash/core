//
// Created by masi on 7/17/23.
//

#include <system_error>
#include "common/utils/cluster_config.h"
#include "storage/storage.h"
#include "deduplicator/deduplicator.h"
#include "directory/directory.h"
#include "entrypoint/entrypoint.h"

#include <config.h>
#include "common/utils/log.h"

using namespace uh::cluster;

void execute_role (const uh::cluster::role role, const std::size_t id, const std::string& registry_url) {

    switch (role) {
        case uh::cluster::STORAGE_SERVICE: {
            LOG_INFO() << "starting storage service";
            uh::cluster::storage ds(id, registry_url);
            ds.run();
            break;
        }

        case uh::cluster::DEDUPLICATOR_SERVICE: {
            LOG_INFO() << "starting deduplicatior service";
            uh::cluster::deduplicator dd (id, registry_url);
            dd.run();
            break;
        }

        case uh::cluster::DIRECTORY_SERVICE: {
            LOG_INFO() << "starting directory service";
            uh::cluster::directory dr (id, registry_url);
            dr.run();
            break;
        }

        case uh::cluster::ENTRYPOINT_SERVICE: {
            LOG_INFO() << "starting entrypoint service";
            uh::cluster::entrypoint en (id, registry_url);
            en.run();
            break;
        }

    }

}

//void handleChange(etcd::Response response) {
//    LOG_INFO() << "action: " << response.action() << ", key: " << response.value().key() << ", value: " << response.value().as_string();
//    }
const std::string default_registry_url = "http://127.0.0.1:2379";

int main (int argc, char* args[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << args[0] << " <role> <id> <optional: registry>" << std::endl;
        std::cerr << "\t<role>\t\t" <<
            get_service_string(uh::cluster::STORAGE_SERVICE) << ", " <<
            get_service_string(uh::cluster::DEDUPLICATOR_SERVICE) << ", " <<
            get_service_string(uh::cluster::DIRECTORY_SERVICE) << ", or " <<
            get_service_string(uh::cluster::ENTRYPOINT_SERVICE) << "." << std::endl;
        std::cerr << "\t<id>\t\t" << "Non-negative integer used to identify service instances of the same role." << std::endl;
        std::cerr << "\t<registry>\t" << "Optionally, a URL to an etcd endpoint can be provided to override the default (\"" <<
            default_registry_url << "\")." << std::endl;
        exit(EINVAL);
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
    std::string registry_url = default_registry_url;
    if(argc == 4) {
        registry_url = std::string(args[4]);
    }
    const auto role = get_service_role (role_str);

    execute_role (role, id, registry_url);
}
