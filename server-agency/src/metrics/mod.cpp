#include "mod.h"
#include <logging/logging_boost.h>

using namespace uh::metrics;

namespace uh::an::metrics
{

// ---------------------------------------------------------------------

struct mod::impl
{
    impl(const uh::metrics::config& config, uh::an::state::mod& persistence);

    uh::metrics::service metrics_service;
    protocol_metrics protocol;
    client_metrics_state client;
};

// ---------------------------------------------------------------------

mod::impl::impl(const uh::metrics::config& config, uh::an::state::mod& state_storage)
    : metrics_service(config),
      protocol(metrics_service),
      client(metrics_service, state_storage.client_metrics())
{
}

// ---------------------------------------------------------------------

mod::mod(const uh::metrics::config& config, uh::an::state::mod& state_storage)
    : m_impl(std::make_unique<impl>(config, state_storage))
{
    INFO << "starting metrics module";
}

// ---------------------------------------------------------------------

mod::~mod() = default;

// ---------------------------------------------------------------------

protocol_metrics& mod::protocol()
{
    return m_impl->protocol;
}

// ---------------------------------------------------------------------

client_metrics_state& mod::client()
{
    return m_impl->client;
}

// ---------------------------------------------------------------------

} // namespace uh::an::metrics
