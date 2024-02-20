/*
 * Copyright The OpenTelemetry Authors
 * SPDX-License-Identifier: Apache-2.0
 */

// Notice: this file has been copied from the master branch of the
// opentelemetry-cpp repo as the support for the boost logging sink has not made
// it to a stable release. The corresponding PR has been merged early February
// 20214: https://github.com/open-telemetry/opentelemetry-cpp-contrib/pull/359

#pragma once

#include <opentelemetry/common/attribute_value.h>
#include <opentelemetry/logs/log_record.h>
#include <opentelemetry/logs/severity.h>

#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/trivial.hpp>
#include <boost/version.hpp>

#include <chrono>
#include <functional>

namespace opentelemetry {
namespace instrumentation {
namespace boost_log {

using IntMapper = std::function<bool(const boost::log::record_view&, int&)>;
using StringMapper =
    std::function<bool(const boost::log::record_view&, std::string&)>;
using TimeStampMapper = std::function<bool(
    const boost::log::record_view&, std::chrono::system_clock::time_point&)>;

using IntSetter = std::function<void(opentelemetry::logs::LogRecord*, int)>;
using StringSetter =
    std::function<void(opentelemetry::logs::LogRecord*, const std::string&)>;
using TimeStampSetter =
    std::function<void(opentelemetry::logs::LogRecord*,
                       const std::chrono::system_clock::time_point&)>;

struct ValueMappers {
    TimeStampMapper ToTimeStamp;
};

class OpenTelemetrySinkBackend : public boost::log::sinks::basic_sink_backend<
                                     boost::log::sinks::concurrent_feeding> {
public:
    static const std::string& libraryVersion() {
        static const std::string kLibraryVersion =
            std::to_string(BOOST_VERSION / 100000) + "." +
            std::to_string(BOOST_VERSION / 100 % 1000) + "." +
            std::to_string(BOOST_VERSION % 100);
        return kLibraryVersion;
    }

    static inline opentelemetry::logs::Severity
    levelToSeverity(boost::log::trivial::severity_level level) noexcept {
        using namespace boost::log::trivial;
        using opentelemetry::logs::Severity;

        switch (level) {
        case severity_level::fatal:
            return Severity::kFatal;
        case severity_level::error:
            return Severity::kError;
        case severity_level::warning:
            return Severity::kWarn;
        case severity_level::info:
            return Severity::kInfo;
        case severity_level::debug:
            return Severity::kDebug;
        case severity_level::trace:
            return Severity::kTrace;
        default:
            return Severity::kInvalid;
        }
    }

    OpenTelemetrySinkBackend() noexcept
        : OpenTelemetrySinkBackend(ValueMappers{}) {}

    explicit OpenTelemetrySinkBackend(const ValueMappers&) noexcept;

    void consume(const boost::log::record_view&);

private:
    ValueMappers mappers_;
    std::array<TimeStampSetter, 2> set_timestamp_if_valid_;
};

} // namespace boost_log
} // namespace instrumentation
} // namespace opentelemetry
