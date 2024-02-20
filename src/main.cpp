#include "common/license/license.h"
#include "common/utils/cluster_config.h"
#include "common/utils/common.h"
#include "common/utils/log.h"
#include "common/utils/signal_handler.h"
#include "deduplicator/deduplicator.h"
#include "directory/directory.h"
#include "entrypoint/entrypoint.h"
#include "storage/storage.h"
#include <CLI/CLI.hpp>
#include <config.h>
#include <system_error>

using namespace uh;
using namespace uh::cluster;

void execute_role(cluster::role role, const service_config& cfg) {

    signal_handler sh;

    auto start_service = [&sh](auto&& service) -> void {
        sh.add_callback([&service]() { service.stop(); });
        service.run();
    };

    try {
        switch (role) {
        case STORAGE_SERVICE:
            return start_service(storage(cfg));
        case DEDUPLICATOR_SERVICE:
            return start_service(deduplicator(cfg));
        case DIRECTORY_SERVICE:
            return start_service(directory(cfg));
        case ENTRYPOINT_SERVICE:
            return start_service(entrypoint(cfg));
        }
    } catch (const std::exception& e) {
        LOG_ERROR() << "Error in executing role: " << e.what();
        sh.stop();
    }
}

void print_vcsid() {
    std::cout << PROJECT_NAME << " " << PROJECT_VERSION << " (" << __DATE__
              << " " << __TIME__ << ")\n"
              << PROJECT_REPOSITORY << " (" << PROJECT_VCSID << ")\n";
    exit(0);
}

int main(int argc, char** argv) {
    try {
        CLI::App app{"UltiHash Object Storage Cluster"};
        argv = app.ensure_utf8(argv);

        service_config cfg;
        cluster::role role;
        boost::log::trivial::severity_level log_level;
        app.add_option("role", role,
                       "service role, i.e. storage, deduplicator, directory, "
                       "or entrypoint")
            ->required()
            ->transform([](const std::string& role_str) {
                return std::to_string(get_service_role(role_str));
            });
        app.add_option(
               "--license,-L",
               [&cfg](CLI::results_t res) {
                   cfg.license = check_license(res[0]);
                   return true;
               },
               "UltiHash license string")
            ->envname(ENV_CFG_LICENSE)
            ->required();
        app.add_option("registry,--registry,-r", cfg.etcd_url,
                       "URL to etcd endpoint")
            ->default_val("http://127.0.0.1:2379");
        app.add_option("working_dir,--workdir,-w", cfg.working_dir,
                       "path to working directory ")
            ->default_val("/var/lib/uh")
            ->check(CLI::ExistingDirectory);
        app.add_option(
               "--log-level,-l", log_level,
               "severity level, i.e. DEBUG, INFO, WARN, ERROR, or FATAL")
            ->transform([](const std::string& severity_str) {
                return std::to_string(
                    uh::log::severity_from_string(severity_str));
            })
            ->default_val(uh::log::to_string(boost::log::trivial::info))
            ->envname(ENV_CFG_LOG_LEVEL);
        app.add_option("--telemetry-endpoint,-e", cfg.telemetry_url,
                       "URL to opentelemetry endpoint")
            ->envname(ENV_CFG_OTEL_ENDPOINT);

        app.add_flag_callback(
            "--vcsid", print_vcsid,
            "Print the VCS commit id this executable was compiled from");

        CLI11_PARSE(app, argc, argv);

        uh::log::config lc{
            .sinks = {uh::log::sink_config{.type = uh::log::sink_type::cout,
                                           .level = log_level},
                      uh::log::sink_config{.type = uh::log::sink_type::file,
                                           .filename = "log.log",
                                           .level = log_level}}};

        log::init(lc);

        LOG_INFO() << "license loaded for " << cfg.license.customer
                   << " -- storage size: " << cfg.license.max_data_store_size
                   << " bytes";

        execute_role(role, cfg);
    } catch (const std::exception& e) {
        std::cerr << "Failure during startup: " << e.what() << "\n";
    }
}
