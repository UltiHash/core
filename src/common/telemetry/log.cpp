#include "log.h"

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

using namespace boost::log;

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

boost::shared_ptr<sinks::sink> make_sink(const sink_config& cfg) {
    auto sink = boost::make_shared<
        sinks::synchronous_sink<sinks::text_ostream_backend>>();

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

trivial::severity_level severity_from_string(const std::string& s) {
    RETURN_IF_MATCH(s, "DEBUG", trivial::debug);
    RETURN_IF_MATCH(s, "INFO", trivial::info);
    RETURN_IF_MATCH(s, "WARN", trivial::warning);
    RETURN_IF_MATCH(s, "ERROR", trivial::error);
    RETURN_IF_MATCH(s, "FATAL", trivial::fatal);

    throw std::runtime_error("unsupported log level type: " + s);
}

// ---------------------------------------------------------------------

std::string to_string(boost::log::trivial::severity_level level) {
    switch (level) {
    case trivial::debug:
        return "DEBUG";
    case trivial::info:
        return "INFO";
    case trivial::warning:
        return "WARN";
    case trivial::error:
        return "ERROR";
    case trivial::fatal:
        return "FATAL";
    case trivial::trace:
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

boost::shared_ptr<sinks::sink> make_otel_sink(const sink_config& cfg) {
    opentelemetry::exporter::otlp::OtlpGrpcLogRecordExporterOptions log_opts;
    log_opts.endpoint = cfg.otel_endpoint;
    auto exporter =
        otel_otlp::OtlpGrpcLogRecordExporterFactory::Create(log_opts);
    auto processor = otel_logs_sdk::SimpleLogRecordProcessorFactory::Create(
        std::move(exporter));
    std::shared_ptr<otel_logs::LoggerProvider> provider(
        otel_logs_sdk::LoggerProviderFactory::Create(std::move(processor)));

    opentelemetry::logs::Provider::SetLoggerProvider(provider);

    auto backend = boost::make_shared<otel_log_sink>();
    auto otel_sink =
        boost::make_shared<boost::log::sinks::synchronous_sink<otel_log_sink>>(
            backend);
    return otel_sink;
}

void init(const config& cfg) {
    logging::add_common_attributes();

    for (const auto& sink : cfg.sinks) {
        if (sink.type == sink_type::otel && !sink.otel_endpoint.empty()) {
            boost::log::core::get()->add_sink(make_otel_sink(sink));
        } else {
            logging::core::get()->add_sink(make_sink(sink));
        }
    }
}

// ---------------------------------------------------------------------

} // namespace uh::log
