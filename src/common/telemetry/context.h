#ifndef CORE_COMMON_TELEMETRY_CONTEXT_H
#define CORE_COMMON_TELEMETRY_CONTEXT_H

#include <boost/asio.hpp>
#include <opentelemetry/sdk/metrics/meter_provider_factory.h>
#include <opentelemetry/trace/span_context.h>

#include <utility>
#include <vector>

namespace uh::cluster {

class context {
public:
    context() = default;
    context(const std::string& name);
    context(const std::vector<char>& buffer);

    context sub_context(const std::string& name);

    template <typename value>
    void set_attribute(const std::string& name, value v) {
        if (m_span) {
            m_span->span->SetAttribute(name, std::move(v));
        }
    }

    void set_name(const std::string& name);

    std::vector<char> serialize() const;

    boost::asio::ip::tcp::endpoint& peer() { return m_peer; }
    const boost::asio::ip::tcp::endpoint& peer() const { return m_peer; }

    static constexpr std::size_t SERIALIZED_SIZE =
        opentelemetry::trace::TraceId::kSize +
        opentelemetry::trace::SpanId::kSize + 2;

    struct span_wrap {
        span_wrap(std::shared_ptr<opentelemetry::trace::Span> span);
        ~span_wrap();
        std::shared_ptr<opentelemetry::trace::Span> span;
    };

private:
    context(std::shared_ptr<span_wrap> span);

    std::shared_ptr<span_wrap> m_span;
    boost::asio::ip::tcp::endpoint m_peer;
};

void initialize_traces_exporter(const std::string& endpoint);

inline thread_local context CURRENT_CONTEXT;

} // namespace uh::cluster

#endif
