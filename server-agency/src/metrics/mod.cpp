#include "mod.h"

using namespace uh::metrics;

namespace uh::an::metrics
{

// ---------------------------------------------------------------------

struct mod::impl
{
    impl(const uh::metrics::config& config, uh::an::persistence::mod& persistence);

    uh::metrics::service metrics_service;
    protocol_metrics protocol;
    client_metrics client;
};

// ---------------------------------------------------------------------

mod::impl::impl(const uh::metrics::config& config, uh::an::persistence::mod& persistence)
    : metrics_service(config),
      protocol(metrics_service),
      client(metrics_service, persistence.clientM_persistence())
{
}

// ---------------------------------------------------------------------

mod::mod(const uh::metrics::config& config, uh::an::persistence::mod& persistence)
    : m_impl(std::make_unique<impl>(config, persistence))
{
    INFO << "             starting metrics module";
}

// ---------------------------------------------------------------------

mod::~mod() = default;

// ---------------------------------------------------------------------

protocol_metrics& mod::protocol()
{
    return m_impl->protocol;
}

// ---------------------------------------------------------------------

client_metrics& mod::client()
{
    return m_impl->client;
}

// ---------------------------------------------------------------------

} // namespace uh::an::metrics
