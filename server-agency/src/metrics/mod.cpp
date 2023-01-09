#include "mod.h"


using namespace uh::metrics;

namespace uh::an::metrics
{

// ---------------------------------------------------------------------

struct mod::impl
{
    impl(const config::options& options);

    uh::metrics::service metrics_service;
    uh::an::metrics::metrics metrics;
};

// ---------------------------------------------------------------------

mod::impl::impl(const config::options& options)
    : metrics_service(options.metrics().config()),
      metrics(metrics_service)
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

uh::an::metrics::metrics& mod::metrics()
{
    return m_impl->metrics;
}

// ---------------------------------------------------------------------

} // namespace uh::an::metrics
