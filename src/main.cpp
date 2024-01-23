//
// Created by masi on 7/17/23.
//

#include <system_error>
#include "common/utils/cluster_config.h"
#include "storage/storage.h"
#include "deduplicator/deduplicator.h"
#include "directory/directory.h"
#include "entrypoint/entrypoint.h"
#include <common/utils/common.h>
#include <config.h>
#include "common/utils/log.h"

using namespace uh::cluster;

void execute_role (const uh::cluster::role role, const std::string& registry_url) {

    switch (role) {
        case uh::cluster::STORAGE_SERVICE: {
            uh::cluster::storage ds(registry_url);
            ds.run();
            break;
        }

        case uh::cluster::DEDUPLICATOR_SERVICE: {
            uh::cluster::deduplicator dd (registry_url);
            dd.run();
            break;
        }

        case uh::cluster::DIRECTORY_SERVICE: {
            uh::cluster::directory dr (registry_url);
            dr.run();
            break;
        }

        case uh::cluster::ENTRYPOINT_SERVICE: {
            uh::cluster::entrypoint en (registry_url);
            en.run();
            break;
        }

    }

}

const std::string default_registry_url = "http://127.0.0.1:2379";

int main (int argc, char* args[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << args[0] << " <role> <optional: registry>" << std::endl;
        std::cerr << "\t<role>\t\t" <<
            get_service_string(uh::cluster::STORAGE_SERVICE) << ", " <<
            get_service_string(uh::cluster::DEDUPLICATOR_SERVICE) << ", " <<
            get_service_string(uh::cluster::DIRECTORY_SERVICE) << ", or " <<
            get_service_string(uh::cluster::ENTRYPOINT_SERVICE) << "." << std::endl;
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

    const auto role_str = std::string(args[1]);
    std::string registry_url = default_registry_url;
    if(argc == 3) {
        registry_url = std::string(args[2]);
    }
    const auto role = get_service_role (role_str);

    //LOG_INFO() << "starting " << PROJECT_NAME << " " << PROJECT_VERSION << " executable on host \""  << boost::asio::ip::host_name() <<
    //    "\" using service role \"" << role_str << "\", service id \"" << id << "\" and service registry endpoints \"" << registry_url << "\"." ;
    execute_role (role, registry_url);
}
