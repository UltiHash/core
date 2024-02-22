#include "config.h"

#include <config.h>

#include <CLI/CLI.hpp>
#include <system_error>

namespace uh::cluster {

namespace {

void print_vcsid() {
    std::cout << PROJECT_NAME << " " << PROJECT_VERSION << " (" << __DATE__
              << " " << __TIME__ << ")\n"
              << PROJECT_REPOSITORY << " (" << PROJECT_VCSID << ")\n";
    exit(0);
}

log::config
make_log_config(const service_config& cfg,
                const boost::log::trivial::severity_level& log_level) {
    log::config lc;

    if (cfg.telemetry_url.empty()) {
        lc = {.sinks = {log::sink_config{.type = log::sink_type::cout,
                                         .level = log_level},
                        log::sink_config{.type = log::sink_type::file,
                                         .filename = "log.log",
                                         .level = log_level}}};
    } else {
        lc = {.sinks = {log::sink_config{.type = log::sink_type::cout,
                                         .level = log_level},
                        log::sink_config{.type = log::sink_type::otel,
                                         .otel_endpoint = cfg.telemetry_url}}};
    }
    return lc;
}

} // namespace

config read_config(int argc, char** argv) {
    CLI::App app{"UltiHash Object Storage Cluster"};
    argv = app.ensure_utf8(argv);

    config rv;

    boost::log::trivial::severity_level log_level;
    app.add_option("role", rv.role,
                   "service role, i.e. storage, deduplicator, directory, "
                   "or entrypoint")
        ->required()
        ->transform([](const std::string& role_str) {
            return std::to_string(get_service_role(role_str));
        });
    app.add_option(
           "--license,-L",
           [&rv](CLI::results_t res) {
               rv.service.license = check_license(res[0]);
               return true;
           },
           "UltiHash license string")
        ->envname(ENV_CFG_LICENSE)
        ->required();
    app.add_option("registry,--registry,-r", rv.service.etcd_url,
                   "URL to etcd endpoint")
        ->default_val("http://127.0.0.1:2379");
    app.add_option("working_dir,--workdir,-w", rv.service.working_dir,
                   "path to working directory ")
        ->default_val("/var/lib/uh")
        ->check(CLI::ExistingDirectory);
    app.add_option("--log-level,-l", log_level,
                   "severity level, i.e. DEBUG, INFO, WARN, ERROR, or FATAL")
        ->transform([](const std::string& severity_str) {
            return std::to_string(uh::log::severity_from_string(severity_str));
        })
        ->default_val(uh::log::to_string(boost::log::trivial::info))
        ->envname(ENV_CFG_LOG_LEVEL);
    app.add_option("--telemetry-endpoint,-e", rv.service.telemetry_url,
                   "URL to opentelemetry endpoint")
        ->envname(ENV_CFG_OTEL_ENDPOINT);

    app.add_flag_callback(
        "--vcsid", print_vcsid,
        "Print the VCS commit id this executable was compiled from");

    app.parse(argc, argv);
    rv.log = make_log_config(rv.service, log_level);

    return rv;
}

} // namespace uh::cluster
