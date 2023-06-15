#include "mod.h"
#include <logging/logging_boost.h>

using namespace uh::metrics;

namespace uh::dbn::metrics
{

// ---------------------------------------------------------------------

struct mod::impl
{
    impl(const uh::metrics::config& config);

    uh::metrics::service metrics_service;
    protocol_metrics protocol;
    storage_metrics storage;
};

// ---------------------------------------------------------------------

mod::impl::impl(const uh::metrics::config& config)
    : metrics_service(config),
      protocol(metrics_service),
      storage(metrics_service)
{
}

// ---------------------------------------------------------------------

mod::mod(const uh::metrics::config& config)
    : m_impl(std::make_unique<impl>(config))
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

storage_metrics& mod::storage()
{
    return m_impl->storage;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::metrics
