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

int main (int argc, char* args[]) {
    if (argc < 3 || argc > 4) {
        throw std::invalid_argument("Usage: uh-cluster <role> <id> <optional: registry URL>");
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

    const auto role_str = std::string(args[1]);
    const std::size_t id = std::stoul(args[2]);
    std::string registry_url = "http://127.0.0.1:2379";
    if(argc == 4) {
        registry_url = std::string(args[4]);
    }
    const auto role = get_service_role (role_str);

    execute_role (role, id, registry_url);
}
