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

void register_service(CLI::App& app, service_config& cfg) {
    auto group = app.add_option_group("service", "service configuration");

    // Note: CLI11 does not allow reading environment variables for options
    // that were added to a group
    // (https://github.com/CLIUtils/CLI11/issues/1013).
    app.add_option(
           "--license,-L",
           [&cfg](CLI::results_t res) {
               cfg.license = check_license(res[0]);
               return true;
           },
           "UltiHash license string")
        ->envname(ENV_CFG_LICENSE)
        ->required();

    group->add_option("--registry,-r", cfg.etcd_url, "URL to etcd endpoint")
        ->default_val(cfg.etcd_url);

    group
        ->add_option("--workdir,-w", cfg.working_dir,
                     "path to working directory ")
        ->default_val(cfg.working_dir)
        ->check(CLI::ExistingDirectory);

    group
        ->add_option("--telemetry-endpoint,-e", cfg.telemetry_url,
                     "URL to opentelemetry endpoint")
        ->envname(ENV_CFG_OTEL_ENDPOINT);
}

void register_server(CLI::App& app, server_config& cfg) {
    auto group = app.add_option_group("server", "server network configuration");

    group
        ->add_option("--server-threads", cfg.threads,
                     "threads handling incoming connections")
        ->default_val(cfg.threads);

    group->add_option("--port,-p", cfg.port, "server listening port")
        ->default_val(cfg.port);

    group
        ->add_option("--address,-A", cfg.bind_address,
                     "server listening address")
        ->default_val(cfg.bind_address);
}

void register_global_data_view(CLI::App& app, global_data_view_config& cfg) {
    auto group =
        app.add_option_group("data view", "storage access configuration");

    group
        ->add_option("--storage-connections",
                     cfg.storage_service_connection_count,
                     "number of connections per storage service")
        ->default_val(cfg.storage_service_connection_count);

    group
        ->add_option("--l1-capacity", cfg.read_cache_capacity_l1,
                     "number of L1 cache entries")
        ->default_val(cfg.read_cache_capacity_l1);

    group
        ->add_option("--l2-capacity", cfg.read_cache_capacity_l2,
                     "number of L2 cache entries")
        ->default_val(cfg.read_cache_capacity_l2);

    group
        ->add_option("--l1-sample-size", cfg.l1_sample_size,
                     "size of samples in L1 cache")
        ->default_val(cfg.l1_sample_size);

    group
        ->add_option("--max-store-size", cfg.max_data_store_size,
                     "maximum size of data store")
        ->default_val(cfg.max_data_store_size);
}

void register_bucket(CLI::App& app, bucket_config& cfg) {
    auto group = app.add_option_group("bucket", "bucket configuration");

    group
        ->add_option("--min-file-size", cfg.min_file_size,
                     "minimum allocated size for buckets")
        ->default_val(cfg.min_file_size);

    group
        ->add_option("--max-file-size", cfg.max_file_size,
                     "maximum allocated size for buckets")
        ->default_val(cfg.max_file_size);

    group
        ->add_option("--max-storage-size", cfg.max_storage_size,
                     "maximum size for buckets")
        ->default_val(cfg.max_storage_size);

    group
        ->add_option("--max-chunk-size", cfg.max_chunk_size,
                     "maximum size for bucket chunks")
        ->default_val(cfg.max_chunk_size);
}

CLI::App* sub_storage(CLI::App& app, storage_config& cfg) {
    auto* rv = app.add_subcommand("storage", "Run as storage service");

    register_server(*rv, cfg.server);

    rv->add_option("--min-file-size", cfg.data_store.min_file_size,
                   "minimum file size in data store")
        ->default_val(cfg.data_store.min_file_size);

    rv->add_option("--max-file-size", cfg.data_store.max_file_size,
                   "maximum file size in data store")
        ->default_val(cfg.data_store.max_file_size);

    rv->add_option("--max-store-size", cfg.data_store.max_data_store_size,
                   "maximum size of data store")
        ->default_val(cfg.data_store.max_data_store_size);

    return rv;
}

CLI::App* sub_entrypoint(CLI::App& app, entrypoint_config& cfg) {

    auto* rv = app.add_subcommand("entrypoint", "Run as entrypoint service");

    register_server(*rv, cfg.server);

    rv->add_option("--dedupe-connections", cfg.dedupe_node_connection_count,
                   "number of connections per deduplication service")
        ->default_val(cfg.dedupe_node_connection_count);

    rv->add_option("--directory-connections", cfg.directory_connection_count,
                   "number of connections per directory service")
        ->default_val(cfg.directory_connection_count);

    rv->add_option("--worker", cfg.worker_thread_count,
                   "number of worker threads")
        ->default_val(cfg.worker_thread_count);

    return rv;
}

CLI::App* sub_directory(CLI::App& app, directory_config& cfg) {

    auto* rv = app.add_subcommand("directory", "Run as directory service");

    register_server(*rv, cfg.server);
    register_global_data_view(*rv, cfg.global_data_view);
    register_bucket(*rv, cfg.directory_store_conf.bucket_conf);

    rv->add_option("--worker-count", cfg.worker_thread_count,
                   "number of worker threads")
        ->default_val(cfg.worker_thread_count);

    rv->add_option("--working-dir", cfg.directory_store_conf.working_dir,
                   "directory persistence directory")
        ->default_val(cfg.directory_store_conf.working_dir);

    rv->add_option("--max-store-size", cfg.max_data_store_size,
                   "maximum size of data store")
        ->default_val(cfg.max_data_store_size);

    return rv;
}

CLI::App* sub_deduplicator(CLI::App& app, deduplicator_config& cfg) {
    auto* rv =
        app.add_subcommand("deduplicator", "Run as deduplicator service");

    register_server(*rv, cfg.server);
    register_global_data_view(*rv, cfg.global_data_view);

    rv->add_option("--working-dir", cfg.working_dir,
                   "directory persistence directory")
        ->default_val(cfg.working_dir);

    rv->add_option("--worker-count", cfg.worker_thread_count,
                   "number of worker threads")
        ->default_val(cfg.worker_thread_count);

    rv->add_option("--min-fragment-size", cfg.min_fragment_size,
                   "minimum fragment size")
        ->default_val(cfg.min_fragment_size);

    rv->add_option("--max-fragment-size", cfg.max_fragment_size,
                   "maximum fragment size")
        ->default_val(cfg.max_fragment_size);

    rv->add_option("--minimum-data-size", cfg.dedupe_worker_minimum_data_size,
                   "minimum worker data size")
        ->default_val(cfg.dedupe_worker_minimum_data_size);

    return rv;
}

} // namespace

std::optional<config> read_config(int argc, char** argv) {
    CLI::App app{"UltiHash Object Storage Cluster"};
    argv = app.ensure_utf8(argv);

    config rv;

    auto sub_str = sub_storage(app, rv.storage);
    auto sub_ep = sub_entrypoint(app, rv.entrypoint);
    auto sub_dir = sub_directory(app, rv.directory);
    auto sub_dd = sub_deduplicator(app, rv.deduplicator);

    register_service(app, rv.service);

    app.require_subcommand(1);

    boost::log::trivial::severity_level log_level;
    app.add_option("--log-level,-l", log_level,
                   "severity level, i.e. DEBUG, INFO, WARN, ERROR, or FATAL")
        ->transform([](const std::string& severity_str) {
            return std::to_string(uh::log::severity_from_string(severity_str));
        })
        ->default_val(uh::log::to_string(boost::log::trivial::info))
        ->envname(ENV_CFG_LOG_LEVEL);

    app.add_flag_callback(
        "--vcsid", print_vcsid,
        "Print the VCS commit id this executable was compiled from");

    try {
        app.parse(argc, argv);
    } catch (const CLI::Success& e) {
        app.exit(e);
        return {};
    }

    if (sub_str->parsed()) {
        rv.role = STORAGE_SERVICE;
    } else if (sub_ep->parsed()) {
        rv.role = ENTRYPOINT_SERVICE;
    } else if (sub_dir->parsed()) {
        rv.role = DIRECTORY_SERVICE;
    } else if (sub_dd->parsed()) {
        rv.role = DEDUPLICATOR_SERVICE;
    } else {
        throw std::runtime_error("unsupported sub command given");
    }

    rv.log = make_log_config(rv.service, log_level);
    rv.directory.max_data_store_size = rv.service.license.max_data_store_size;

    return rv;
}

} // namespace uh::cluster
