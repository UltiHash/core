#include "license_package.h"
#include "demo_license.h"

#include <util/exception.h>
#include <logging/logging_boost.h>


namespace uh::licensing
{

namespace
{

// ---------------------------------------------------------------------

std::unique_ptr<backend> mk_backend(const config& c)
{
#ifdef USE_LICENSE_SPRING
    try{
        if (!c.activation_key.empty())
        {
            switch (c.type)
            {
                case uh::licensing::config::backend_type::license_spring:
                    INFO << "Loading standard license with key.";
                    return std::make_unique<license_spring>(c.config, c.activation_key);
                default:
                    THROW(util::exception, "The demo license does not require a key!");

            }

        }

        switch (c.type)
        {
            case uh::licensing::config::backend_type::license_spring:
                INFO << "Loading standard license.";
                return std::make_unique<license_spring>(c.config);
            default:
                INFO << "Loading demo license.";
                return std::make_unique<demo_license>();

        }
    }
    catch (std::exception& e){
        ERROR << "Licensing check failed for this reason: " << e.what();
        INFO << "Loading demo license.";
        return std::make_unique<demo_license>();
    }
#else
    return std::make_unique<demo_license>();
#endif
}

// ---------------------------------------------------------------------

}

// ---------------------------------------------------------------------

license_package::license_package(const config& c)
    : m_backend(mk_backend(c))
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

} // namespace uh::licensing
