#include "log.h"
#include "common/types/magic_enum.hpp"
#include "common/utils/common.h"

#include <config.h>

#include <boost/core/null_deleter.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <opentelemetry/exporters/otlp/otlp_grpc_log_record_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_log_record_exporter_options.h>
#include <opentelemetry/logs/provider.h>
#include <opentelemetry/sdk/logs/logger_provider.h>
#include <opentelemetry/sdk/logs/logger_provider_factory.h>
#include <opentelemetry/sdk/logs/processor.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor_factory.h>

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace expr = boost::log::expressions;

namespace otel_otlp = opentelemetry::exporter::otlp;
namespace otel_logs_sdk = opentelemetry::sdk::logs;
namespace otel_logs = opentelemetry::logs;

namespace uh::log {

namespace {

// ---------------------------------------------------------------------

boost::shared_ptr<std::ostream> open_file(const std::filesystem::path& path) {
    auto rv = boost::make_shared<std::ofstream>(path);

    if (!*rv) {
        throw std::runtime_error(std::string("could not open file: ") +
                                 path.string());
    }

    return rv;
}

// ---------------------------------------------------------------------

boost::shared_ptr<std::ostream> open_stream(const sink_config& cfg) {
    switch (cfg.type) {
    case sink_type::file:
        return open_file(*cfg.filename);
    case sink_type::clog:
        return {&std::clog, boost::null_deleter()};
    case sink_type::cerr:
        return {&std::cerr, boost::null_deleter()};
    case sink_type::cout:
        return {&std::cout, boost::null_deleter()};
    case sink_type::otel:
        return {};
    }

    throw std::runtime_error("unsupported log sink type");
}

// ---------------------------------------------------------------------

void initialize_otel_log_exporter(const sink_config& cfg) {
    opentelemetry::exporter::otlp::OtlpGrpcLogRecordExporterOptions log_opts;
    log_opts.endpoint = cfg.otel_endpoint;
    auto exporter =
        otel_otlp::OtlpGrpcLogRecordExporterFactory::Create(log_opts);
    auto processor = otel_logs_sdk::SimpleLogRecordProcessorFactory::Create(
        std::move(exporter));
    auto resource = opentelemetry::sdk::resource::Resource::Create(
        {{"service.name", PROJECT_NAME},
         {"service.version", PROJECT_VERSION},
         {"service.role",
          std::string(magic_enum::enum_name(uh::cluster::service_role))}});
    std::shared_ptr<otel_logs::LoggerProvider> provider(
        otel_logs_sdk::LoggerProviderFactory::Create(std::move(processor),
                                                     resource));

    opentelemetry::logs::Provider::SetLoggerProvider(provider);
}

boost::shared_ptr<logging::sinks::sink> make_sink(const sink_config& cfg) {
    if (cfg.type == sink_type::otel && !cfg.otel_endpoint.empty()) {
        initialize_otel_log_exporter(cfg);
        return boost::make_shared<
            logging::sinks::synchronous_sink<otel_log_sink>>(
            boost::make_shared<otel_log_sink>());
    } else {
        auto sink = boost::make_shared<logging::sinks::synchronous_sink<
            logging::sinks::text_ostream_backend>>();

        sink->locked_backend()->add_stream(open_stream(cfg));
        sink->set_filter(logging::trivial::severity >= cfg.level);

        sink->set_formatter(expr::stream
                            << expr::format_date_time<boost::posix_time::ptime>(
                                   "TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
                            << "\t" << logging::trivial::severity << "\t"
                            << expr::smessage);

        sink->locked_backend()->auto_flush();

        return sink;
    }
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

std::string to_string(sink_type type) {
    switch (type) {
    case sink_type::file:
        return "sink_type::file";
    case sink_type::clog:
        return "sink_type::clog";
    case sink_type::cerr:
        return "sink_type::cerr";
    case sink_type::cout:
        return "sink_type::cout";
    case sink_type::otel:
        return "sink_type::otel";
    }

    throw std::runtime_error("unsupported log sink type");
}

// ---------------------------------------------------------------------

#define RETURN_IF_MATCH(s, string, symbol)                                     \
    if (s == string) {                                                         \
        return symbol;                                                         \
    }

// ---------------------------------------------------------------------

logging::trivial::severity_level severity_from_string(const std::string& s) {
    RETURN_IF_MATCH(s, "DEBUG", logging::trivial::debug);
    RETURN_IF_MATCH(s, "INFO", logging::trivial::info);
    RETURN_IF_MATCH(s, "WARN", logging::trivial::warning);
    RETURN_IF_MATCH(s, "ERROR", logging::trivial::error);
    RETURN_IF_MATCH(s, "FATAL", logging::trivial::fatal);

    throw std::runtime_error("unsupported log level type: " + s);
}

// ---------------------------------------------------------------------

std::string to_string(boost::log::trivial::severity_level level) {
    switch (level) {
    case logging::trivial::debug:
        return "DEBUG";
    case logging::trivial::info:
        return "INFO";
    case logging::trivial::warning:
        return "WARN";
    case logging::trivial::error:
        return "ERROR";
    case logging::trivial::fatal:
        return "FATAL";
    case logging::trivial::trace:
        return "TRIVIAL";
    }

    throw std::runtime_error("unsupported log level type");
}

// ---------------------------------------------------------------------

std::ostream& operator<<(std::ostream& out, const sink_config& c) {
    out << "sink(" << to_string(c.type) << ", "
        << (c.filename ? *c.filename : "<empty>") << ", "
        << uh::log::to_string(c.level) << ")";

    return out;
}

// ---------------------------------------------------------------------

void init(const config& cfg) {
    logging::add_common_attributes();

    for (const auto& sink : cfg.sinks) {
        logging::core::get()->add_sink(make_sink(sink));
    }
}

// ---------------------------------------------------------------------

} // namespace uh::log
