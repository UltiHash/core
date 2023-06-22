//
// Created by benjamin-elias on 06.06.23.
//

#include "license_package.h"

#include <util/exception.h>
#include <logging/logging_boost.h>


namespace uh::licensing
{

// ---------------------------------------------------------------------

license_package::license_package(std::shared_ptr<backend> backend)
    : m_backend(backend)
{
}

// ---------------------------------------------------------------------

bool license_package::check(feature f) const
{
    return m_backend->has_feature(f);
}

// ---------------------------------------------------------------------

void license_package::require(feature f, std::size_t value) const
{
    auto min = m_backend->feature_arg_size_t(f, "min");
    auto max = m_backend->feature_arg_size_t(f, "max");

    if (min > value || max < value)
    {
        THROW(util::exception, "feature " + to_string(f) + " out of bounds");
    }

    try
    {
        auto max_soft = m_backend->feature_arg_size_t(f, "max_soft");
        if (value > max_soft)
        {
            WARNING << "feature " << to_string(f) << " is about to expire";
        }
    }
    catch (const std::exception&)
    {
    }
}

// ---------------------------------------------------------------------

bool license_package::valid()
{
    return true;
}

// ---------------------------------------------------------------------

} // namespace uh::licensing
