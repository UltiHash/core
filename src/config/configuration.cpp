#include "configuration.h"
#include "config.h"
#include <CLI/CLI.hpp>

namespace uh::cluster {

namespace {

void print_vcsid() {
    std::cout << PROJECT_NAME << " " << PROJECT_VERSION << " (" << __DATE__
              << " " << __TIME__ << ")\n"
              << PROJECT_REPOSITORY << " (" << PROJECT_VCSID << ")\n";
    exit(0);
}

std::list<std::filesystem::path> split_paths(std::string str) {
    size_t pos;
    std::list<std::filesystem::path> paths;
    do {
        pos = str.find(CONFIG_PATH_DELIMETER);
        paths.emplace_back(str.substr(0, pos));
        str.erase(0, pos + CONFIG_PATH_DELIMETER.length());
    } while (pos != std::string::npos);
    return paths;
}

std::string working_dir_validator(const std::string& arg) {
    for (const auto& p : split_paths(arg)) {
        if (!std::filesystem::exists(p)) {
            return "Non-existing working dir path";
        }
    }
    return "";
}

log::config
make_log_config(const service_config& cfg,
                const boost::log::trivial::severity_level& log_level,
                const uh::cluster::role service_role) {
    log::config lc;

    if (cfg.telemetry_url.empty()) {
        lc = {.sinks = {log::sink_config{.type = log::sink_type::cout,
                                         .level = log_level,
                                         .service_role = service_role}}};
    } else {
        lc = {.sinks = {log::sink_config{.type = log::sink_type::cout,
                                         .level = log_level,
                                         .service_role = service_role},
                        log::sink_config{.type = log::sink_type::otel,
                                         .otel_endpoint = cfg.telemetry_url,
                                         .level = log_level,
                                         .service_role = service_role}}};
    }
    return lc;
}

void register_service(CLI::App& app, service_config& cfg) {
    auto group = app.add_option_group("service", "service configuration");

    group
        ->add_option("--registry,-r", cfg.etcd_config.url,
                     "URL to etcd endpoint")
        ->default_val(cfg.etcd_config.url);
    group
        ->add_option("--registry-user", cfg.etcd_config.username,
                     "username for etcd authentication")
        ->envname(ENV_CFG_ETCD_USERNAME);
    group
        ->add_option("--registry-pass", cfg.etcd_config.password,
                     "password for etcd authentication")
        ->envname(ENV_CFG_ETCD_PASSWORD);

    group
        ->add_option("--workdir,-w", cfg.working_dir,
                     "path to working directory ")
        ->default_val(cfg.working_dir)
        ->check(working_dir_validator)
        ->envname(UH_WORKING_DIR);

    app.add_option("--telemetry-endpoint,-e", cfg.telemetry_url,
                   "URL to opentelemetry endpoint")
        ->envname(ENV_CFG_OTEL_ENDPOINT);

    app.add_option("--telemetry-interval", cfg.telemetry_interval,
                   "interval of telemetry exports in milliseconds")
        ->default_val(cfg.telemetry_interval)
        ->envname(ENV_CFG_OTEL_EXPORT_INTERVAL);

    app.add_flag("--enable-traces", cfg.enable_traces,
                 "enable generation of traces")
        ->default_val(cfg.enable_traces)
        ->envname(ENV_CFG_ENABLE_TRACES);
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
        ->add_option("--l2-capacity", cfg.read_cache_capacity_l2,
                     "number of L2 cache entries")
        ->default_val(cfg.read_cache_capacity_l2);

    group
        ->add_option(
            "--ec-data-shards", cfg.ec_data_shards,
            "number of data shards (K) in an erasure-coded storage group")
        ->default_val(cfg.ec_data_shards);

    group
        ->add_option(
            "--ec-parity-shards", cfg.ec_parity_shards,
            "number of parity shards (M) in an erasure-coded storage group")
        ->default_val(cfg.ec_parity_shards);
}

CLI::App* sub_storage(CLI::App& app, storage_config& cfg) {
    auto* rv = app.add_subcommand("storage", "Run as storage service");

    register_server(*rv, cfg.server);

    rv->add_option("--file-size", cfg.data_store.max_file_size,
                   "minimum file size in data store")
        ->default_val(cfg.data_store.max_file_size);

    rv->add_option("--max-store-size", cfg.data_store.max_data_store_size,
                   "maximum size of data store")
        ->default_val(cfg.data_store.max_data_store_size);

    return rv;
}

CLI::App* sub_entrypoint(CLI::App& app, entrypoint_config& cfg) {

    auto* rv = app.add_subcommand("entrypoint", "Run as entrypoint service");

    register_server(*rv, cfg.server);
    register_global_data_view(*rv, cfg.global_data_view);

    rv->add_option("--dedupe-connections", cfg.dedupe_node_connection_count,
                   "number of connections per deduplication service")
        ->default_val(cfg.dedupe_node_connection_count);

    rv->add_option("--worker", cfg.worker_thread_count,
                   "number of worker threads")
        ->default_val(cfg.worker_thread_count);

    rv->add_option("--buffer-size", cfg.buffer_size,
                   "buffer size before sending data to deduplicators")
        ->default_val(cfg.buffer_size);

    rv->add_flag("--no-dedupe", cfg.noop_deduplicator,
                 "disable deduplication and write directly to storage")
        ->default_val(cfg.noop_deduplicator)
        ->envname(ENV_CFG_NO_DEDUPE);

    configure(*rv, cfg.database);

    return rv;
}

CLI::App* sub_deduplicator(CLI::App& app, deduplicator_config& cfg) {
    auto* rv =
        app.add_subcommand("deduplicator", "Run as deduplicator service");

    register_server(*rv, cfg.server);
    register_global_data_view(*rv, cfg.global_data_view);

    rv->add_option("--worker-count", cfg.worker_thread_count,
                   "number of worker threads")
        ->default_val(cfg.worker_thread_count);

    rv->add_option("--min-fragment-size", cfg.min_fragment_size,
                   "minimum fragment size")
        ->default_val(cfg.min_fragment_size);

    rv->add_option("--max-fragment-size", cfg.max_fragment_size,
                   "maximum fragment size")
        ->default_val(cfg.max_fragment_size);

    rv->add_option("--set-capacity", cfg.set_capacity,
                   "maximum number of fragments in the dedupe set")
        ->default_val(cfg.set_capacity);

    return rv;
}

CLI::App* sub_coordinator(CLI::App& app, coordinator_config& cfg) {
    auto* rv = app.add_subcommand("coordinator", "Run as coordinator service");
    rv->add_option("--thread-count", cfg.thread_count, "number of threads")
        ->default_val(cfg.thread_count);

    app.add_option(
           "--license,-L",
           [&cfg](CLI::results_t res) {
               cfg.license = license::create(res[0]);
               return true;
           },
           "UltiHash license json-string")
        ->envname(ENV_CFG_LICENSE_JSON)
        ->default_val(cfg.license);

    app.add_option("--backend-host", cfg.backend_config.backend_host,
                   "backend host")
        ->envname(ENV_CFG_BACKEND_HOST);
    app.add_option("--customer-id", cfg.backend_config.customer_id,
                   "customer ID required to connect to the backend")
        ->envname(ENV_CFG_CUSTOMER_ID);
    app.add_option("--access-token", cfg.backend_config.access_token,
                   "access token required to connect to the backend")
        ->envname(ENV_CFG_ACCESS_TOKEN);
    return rv;
}

} // namespace

std::optional<config> read_config(int argc, char** argv) {
    CLI::App app{"UltiHash Object Storage Cluster"};
    argv = app.ensure_utf8(argv);

    config rv;

    auto sub_str = sub_storage(app, rv.storage);
    auto sub_ep = sub_entrypoint(app, rv.entrypoint);
    auto sub_dd = sub_deduplicator(app, rv.deduplicator);
    auto sub_rk = sub_coordinator(app, rv.coordinator);

    auto sub_dd_str =
        sub_storage(*sub_dd, rv.deduplicator.m_attached_storage.emplace());
    auto sub_en_dd = sub_deduplicator(
        *sub_ep, rv.entrypoint.m_attached_deduplicator.emplace());

    register_service(app, rv.service);

    app.require_subcommand(1);

    boost::log::trivial::severity_level log_level = boost::log::trivial::info;
    configure(app, log_level);

    app.add_flag_callback(
        "--vcsid", print_vcsid,
        "Print the VCS commit id this executable was compiled from");

    try {
        app.parse(argc, argv);
    } catch (const CLI::Success& e) {
        app.exit(e);
        return {};
    }

    auto working_dirs = split_paths(rv.service.working_dir);

    if (sub_str->parsed()) {
        rv.role = STORAGE_SERVICE;

        rv.storage.m_data_store_roots = working_dirs;
        for (auto& p : rv.storage.m_data_store_roots) {
            p /= "storage";
        }

        rv.service.working_dir = rv.storage.m_data_store_roots.front();

    } else if (sub_ep->parsed()) {
        rv.role = ENTRYPOINT_SERVICE;
    } else if (sub_dd->parsed()) {

        rv.role = DEDUPLICATOR_SERVICE;
        if (working_dirs.size() != 1) {
            throw std::invalid_argument(
                "Deduplicator does not support multiple working directories");
        }
        rv.deduplicator.working_dir =
            std::filesystem::path(rv.service.working_dir) / "deduplicator";

    } else if (sub_rk->parsed()) {
        rv.role = COORDINATOR_SERVICE;
        auto& cfg = rv.coordinator;

        if (!cfg.license && !cfg.backend_config) {
            LOG_INFO() << "license: " << cfg.license;
            LOG_INFO() << "backend_host: " << cfg.backend_config.backend_host;
            throw std::invalid_argument("Either a test license or backend "
                                        "configuration must be provided.");
        }

    } else {
        throw std::runtime_error("unsupported sub command given");
    }

    rv.log = make_log_config(rv.service, log_level, rv.role);

    if (!sub_dd_str->parsed()) {
        rv.deduplicator.m_attached_storage.reset();
    }
    if (!sub_en_dd->parsed()) {
        rv.entrypoint.m_attached_deduplicator.reset();
    }

    return rv;
}

void configure(CLI::App& app, db::config& cfg) {
    app.add_option("--db-host,-D", cfg.host_port,
                   "PGSQL server address as HOST:PORT")
        ->default_val(cfg.host_port)
        ->envname(ENV_CFG_DB_HOSTPORT);

    app.add_option("--db-user", cfg.username, "PGSQL user name")
        ->default_val(cfg.username)
        ->envname(ENV_CFG_DB_USER);

    app.add_option("--db-pass", cfg.password, "PGSQL password")
        ->default_val(cfg.password)
        ->envname(ENV_CFG_DB_PASS);

    app.add_option("--db-directory-connections", cfg.directory.count,
                   "Number of connections to directory database")
        ->default_val(cfg.directory.count)
        ->envname(ENV_CFG_DB_DIRECTORY_CONNECTIONS);

    app.add_option("--db-multipart-connections", cfg.multipart.count,
                   "Number of connections to multipart database")
        ->default_val(cfg.multipart.count)
        ->envname(ENV_CFG_DB_MULTIPART_CONNECTIONS);

    app.add_option("--db-users-connections", cfg.users.count,
                   "Number of connections to users database")
        ->default_val(cfg.users.count)
        ->envname(ENV_CFG_DB_USERS_CONNECTIONS);
}

void configure(CLI::App& app, boost::log::trivial::severity_level& log_level) {
    app.add_option("--log-level,-l", log_level,
                   "severity level, i.e. DEBUG, INFO, WARN, ERROR, or FATAL")
        ->transform([](const std::string& severity_str) {
            return std::to_string(uh::log::severity_from_string(severity_str));
        })
        ->default_val(uh::log::to_string(log_level))
        ->envname(ENV_CFG_LOG_LEVEL);
}

} // namespace uh::cluster
