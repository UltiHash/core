#include "mod.h"


using namespace uh::metrics;

namespace uh::an::metrics
{

// ---------------------------------------------------------------------

struct mod::impl
{
    impl(const uh::metrics::config& config);

    uh::metrics::service metrics_service;
    protocol_metrics protocol;
};

// ---------------------------------------------------------------------

mod::impl::impl(const uh::metrics::config& config)
    : metrics_service(config),
      protocol(metrics_service)
{
}

// ---------------------------------------------------------------------

mod::mod(const uh::metrics::config& config)
    : m_impl(std::make_unique<impl>(config))
{
}

// ---------------------------------------------------------------------

mod::~mod() = default;

// ---------------------------------------------------------------------

protocol_metrics& mod::protocol()
{
    return m_impl->protocol;
}

// ---------------------------------------------------------------------

} // namespace uh::an::metrics
