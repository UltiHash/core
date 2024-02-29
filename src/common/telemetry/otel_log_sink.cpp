#include "otel_log_sink.h"

#include <opentelemetry/logs/log_record.h>
#include <opentelemetry/logs/provider.h>

#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/version.hpp>

std::string boost_version() {
    static const std::string library_version =
        std::to_string(BOOST_VERSION / 100000) + "." +
        std::to_string(BOOST_VERSION / 100 % 1000) + "." +
        std::to_string(BOOST_VERSION % 100);
    return library_version;
}

opentelemetry::logs::Severity
convert_severity(boost::log::trivial::severity_level level) noexcept {
    switch (level) {
    case boost::log::trivial::severity_level::fatal:
        return opentelemetry::logs::Severity::kFatal;
    case boost::log::trivial::severity_level::error:
        return opentelemetry::logs::Severity::kError;
    case boost::log::trivial::severity_level::warning:
        return opentelemetry::logs::Severity::kWarn;
    case boost::log::trivial::severity_level::info:
        return opentelemetry::logs::Severity::kInfo;
    case boost::log::trivial::severity_level::debug:
        return opentelemetry::logs::Severity::kDebug;
    case boost::log::trivial::severity_level::trace:
        return opentelemetry::logs::Severity::kTrace;
    default:
        return opentelemetry::logs::Severity::kInvalid;
    }
}

namespace uh::log {

constexpr boost::posix_time::ptime epoch_time(boost::gregorian::date(1970, 1,
                                                                     1));
constexpr boost::posix_time::ptime invalid_time{};
constexpr auto logger_name = "Boost logger";
constexpr auto library_name = "Boost.Log";

void otel_log_sink::consume(const boost::log::record_view& record) {
    auto provider = opentelemetry::logs::Provider::GetLoggerProvider();
    auto logger =
        provider->GetLogger(logger_name, library_name, boost_version());
    auto log_record = logger->CreateLogRecord();

    if (log_record) {
        log_record->SetBody(boost::log::extract_or_default<std::string>(
            record["Message"], std::string{}));
        const auto severity =
            boost::log::extract_or_default<boost::log::trivial::severity_level>(
                record["Severity"], boost::log::trivial::severity_level::debug);
        log_record->SetSeverity(convert_severity(severity));

        const auto timestamp =
            boost::log::extract_or_default<boost::posix_time::ptime>(
                record["TimeStamp"], invalid_time);

        const auto value =
            std::chrono::system_clock::time_point(std::chrono::nanoseconds(
                (timestamp - epoch_time).total_nanoseconds()));
        log_record->SetTimestamp(value);

        logger->EmitLogRecord(std::move(log_record));
    } else {
        std::cerr << "Failed emit log record to OpenTelemetry endpoint."
                  << std::endl;
    }
}

} // namespace uh::log
