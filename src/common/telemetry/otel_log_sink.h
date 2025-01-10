#pragma once

#include <boost/log/sinks/basic_sink_backend.hpp>

namespace uh::log {

class otel_log_sink : public boost::log::sinks::basic_sink_backend<
                          boost::log::sinks::concurrent_feeding> {
public:
    void consume(const boost::log::record_view&);
};

} // namespace uh::log
