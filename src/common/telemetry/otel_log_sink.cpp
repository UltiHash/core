/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

// Notice: this file has been copied from the master branch of the
// opentelemetry-cpp repo as the support for the boost logging sink has not made
// it to a stable release. The corresponding PR has been merged early February
// 20214: https://github.com/open-telemetry/opentelemetry-cpp-contrib/pull/359

#include "otel_log_sink.h"

#include <opentelemetry/logs/provider.h>
#include <opentelemetry/trace/semantic_conventions.h>

#include <boost/log/attributes/value_extraction.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace opentelemetry {
namespace instrumentation {
namespace boost_log {

static bool
ToTimestampDefault(const boost::log::record_view& record,
                   std::chrono::system_clock::time_point& value) noexcept {
    using namespace std::chrono;
    static constexpr boost::posix_time::ptime kEpochTime(
        boost::gregorian::date(1970, 1, 1));
    static constexpr boost::posix_time::ptime kInvalid{};

    const auto& timestamp =
        boost::log::extract_or_default<boost::posix_time::ptime>(
            record["TimeStamp"], kInvalid);
    value = system_clock::time_point(
        nanoseconds((timestamp - kEpochTime).total_nanoseconds()));
    return timestamp != kInvalid;
}

OpenTelemetrySinkBackend::OpenTelemetrySinkBackend(
    const ValueMappers& mappers) noexcept {
    mappers_.ToTimeStamp =
        mappers.ToTimeStamp ? mappers.ToTimeStamp : ToTimestampDefault;

    using namespace opentelemetry::trace::SemanticConventions;
    using opentelemetry::logs::LogRecord;
    using timestamp_t = std::chrono::system_clock::time_point;

    set_timestamp_if_valid_ = {
        [](LogRecord*, const timestamp_t&) {},
        [](LogRecord* log_record, const timestamp_t& timestamp) {
            log_record->SetTimestamp(timestamp);
        }};
}

void OpenTelemetrySinkBackend::consume(const boost::log::record_view& record) {
    static constexpr auto kLoggerName = "Boost logger";
    static constexpr auto kLibraryName = "Boost.Log";

    auto provider = opentelemetry::logs::Provider::GetLoggerProvider();
    auto logger =
        provider->GetLogger(kLoggerName, kLibraryName, libraryVersion());
    auto log_record = logger->CreateLogRecord();

    if (log_record) {
        log_record->SetBody(boost::log::extract_or_default<std::string>(
            record["Message"], std::string{}));
        auto severity =
            boost::log::extract_or_default<boost::log::trivial::severity_level>(
                record["Severity"], boost::log::trivial::severity_level::debug);
        log_record->SetSeverity(levelToSeverity(severity));

        std::chrono::system_clock::time_point timestamp;
        set_timestamp_if_valid_[mappers_.ToTimeStamp(record, timestamp)](
            log_record.get(), timestamp);

        logger->EmitLogRecord(std::move(log_record));
    }
}

} // namespace boost_log
} // namespace instrumentation
} // namespace opentelemetry
