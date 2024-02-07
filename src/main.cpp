#include <system_error>
#include "common/utils/cluster_config.h"
#include "storage/storage.h"
#include "deduplicator/deduplicator.h"
#include "directory/directory.h"
#include "entrypoint/entrypoint.h"
#include <common/utils/common.h>
#include <config.h>
#include "common/utils/log.h"
#include "common/utils/signal_handler.h"

using namespace uh::cluster;

struct config {
    enum action {
        start_service,
        print_vcsid
    };

    static constexpr const char* default_registry_url = "http://127.0.0.1:2379";
    static constexpr const char* default_working_dir = "ultihash_root";

    action task;
    uh::cluster::role role;
    std::size_t id;
    std::string etcd_url;
    std::filesystem::path working_dir;
};

void execute_role(const config& cfg) {

    signal_handler sh;

    auto start_service = [&sh] (auto&& service) -> void {
        sh.add_callback([&service]() { service.stop(); });
        service.run ();
    };

    switch (cfg.role) {
        case STORAGE_SERVICE:
            return start_service(storage(cfg.etcd_url, cfg.working_dir));
        case DEDUPLICATOR_SERVICE:
            return start_service (deduplicator(cfg.etcd_url, cfg.working_dir));
        case DIRECTORY_SERVICE:
            return start_service (directory(cfg.etcd_url, cfg.working_dir));
        case ENTRYPOINT_SERVICE:
            return start_service (entrypoint(cfg.etcd_url, cfg.working_dir));
    }
}

std::optional<config> parse_command_line(int argc, const char* args[]) {

    if (argc > 1 && std::string(args[1]) == "--vcsid") {
        return config{ .task = config::print_vcsid };
    }

    if (argc < 2 || argc > 4) {
        return {};
    }

    return config{
        .task = config::start_service,
        .role = get_service_role(args[1]),
        .etcd_url = argc == 3 ? args[2] : config::default_registry_url,
        .working_dir = argc == 4 ? args[3] : config::default_working_dir,
    };
}

void print_help(std::ostream& out) {
    out << "Usage: " << PROJECT_NAME << " <role> [registry] [working_dir]\n"
        << "\trole\t\t" << "service role, ie. "
            << get_service_string(uh::cluster::STORAGE_SERVICE) << ", "
            << get_service_string(uh::cluster::DEDUPLICATOR_SERVICE) << ", "
            << get_service_string(uh::cluster::DIRECTORY_SERVICE) << ", or "
            << get_service_string(uh::cluster::ENTRYPOINT_SERVICE) << "\n"
        << "\tregistry\t" << "URL to etcd endpoint (default: " << config::default_registry_url << ")\n"
        << "\tworking_dir\t" << "path to working directory (default: " << config::default_working_dir << ")\n";
}

void print_vcsid(std::ostream& out) {
    out << PROJECT_NAME << " " << PROJECT_VERSION << " (" << __DATE__ << " " << __TIME__ << ")\n"
        << PROJECT_REPOSITORY << " (" << PROJECT_VCSID << ")\n";
}

int main (int argc, const char* args[]) {
    try {
        auto cfg = parse_command_line(argc, args);
        if (!cfg) {
            print_help(std::cerr);
            return 1;
        }

        if (cfg->task == config::print_vcsid) {
            print_vcsid(std::cout);
            return 0;
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

        execute_role(*cfg);
    } catch (const std::exception& e) {
        std::cerr << "Failure during startup: " << e.what() << "\n";
    }
}
