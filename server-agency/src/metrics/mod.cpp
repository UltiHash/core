#include "mod.h"


using namespace uh::metrics;

namespace uh::an::metrics
{

// ---------------------------------------------------------------------

struct mod::impl
{
    impl(const config::options& options);

    uh::metrics::service metrics_service;
    protocol_metrics protocol;
};

// ---------------------------------------------------------------------

mod::impl::impl(const config::options& options)
    : metrics_service(options.metrics().config()),
      protocol(metrics_service)
{
}

// ---------------------------------------------------------------------

mod::mod(const config::options& options)
    : m_impl(std::make_unique<impl>(options))
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
