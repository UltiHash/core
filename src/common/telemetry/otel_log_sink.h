#ifndef UH_LOGGING_OTEL_LOG_SINK_H
#define UH_LOGGING_OTEL_LOG_SINK_H

#include <opentelemetry/logs/severity.h>

#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/trivial.hpp>

#include <string>

namespace uh::log {

class otel_log_sink : public boost::log::sinks::basic_sink_backend<
                          boost::log::sinks::concurrent_feeding> {
public:
    void consume(const boost::log::record_view&);

private:
    static const std::string boost_version();
    static opentelemetry::logs::Severity
    convert_severity(boost::log::trivial::severity_level level) noexcept;
};

} // namespace uh::log

#endif // UH_LOGGING_OTEL_LOG_SINK_H