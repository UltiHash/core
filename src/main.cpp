#include <common/telemetry/log.h>
#include <common/telemetry/trace/trace.h>
#include <common/utils/service_runner.h>
#include <common/utils/strings.h>
#include <config/configuration.h>

#include <coordinator/service.h>
#include <deduplicator/service.h>
#include <entrypoint/service.h>
#include <storage/service.h>
#include <proxy/service.h>

using namespace uh;
using namespace uh::cluster;

static std::any make_service(boost::asio::io_context& ioc, const config& c) {
    switch (c.role) {
    case STORAGE_SERVICE:
        return std::make_shared<storage::service>( //
            ioc, c.service, c.storage);
    case DEDUPLICATOR_SERVICE:
        return std::make_shared<deduplicator::service>( //
            ioc, c.service, c.deduplicator);
    case ENTRYPOINT_SERVICE:
        return std::make_shared<ep::service>( //
            ioc, c.service, c.entrypoint);
    case COORDINATOR_SERVICE:
        return std::make_shared<coordinator::service>( //
            ioc, c.service, c.coordinator);
    case PROXY_SERVICE:
        return std::make_shared<proxy::service>(ioc, c.proxy);
    default:
        throw std::runtime_error("unknown service role: " + serialize(c.role));
    }
}

static std::size_t get_num_threads(const config& c) {
    switch (c.role) {
    case STORAGE_SERVICE:
        return c.storage.num_threads;
    case DEDUPLICATOR_SERVICE:
        return c.deduplicator.num_threads;
    case ENTRYPOINT_SERVICE:
        return c.entrypoint.num_threads;
    case COORDINATOR_SERVICE:
        return c.coordinator.num_threads;
    case PROXY_SERVICE:
        return c.proxy.num_threads;
    default:
        throw std::runtime_error("unknown service role: " + serialize(c.role));
    }
}

int main(int argc, char** argv) {

    try {
        auto config = read_config(argc, argv);
        if (!config) {
            return 0;
        }

        global_service_role = config->role;

        log::init(config->log);

        LOG_INFO() << "starting " << PROJECT_NAME << " " << PROJECT_VERSION
                   << " [" << PROJECT_VCSID << "], running as "
                   << magic_enum::enum_name(global_service_role);

        initialize_metrics_exporter(config->service.telemetry_url,
                                    config->service.telemetry_interval);
        if (config->service.enable_traces &&
            !config->service.telemetry_url.empty()) {
            LOG_DEBUG() << "trace endpoint: " << config->service.telemetry_url;
            initialize_trace(PROJECT_NAME, PROJECT_VERSION,
                             config->service.telemetry_url);
        }
        auto runner = service_runner(
            [&](boost::asio::io_context& ioc) {
                return make_service(ioc, *config);
            },
            get_num_threads(*config));
        runner.run();
    } catch (const std::exception& e) {
        std::cerr << "Failure during startup: " << e.what() << "\n";
    }
}
