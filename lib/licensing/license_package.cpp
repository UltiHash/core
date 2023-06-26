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
    try{
        if (!c.activation_key.empty())
        {
            switch (c.type)
            {
                case uh::licensing::config::backend_type::license_spring:
                    return std::make_unique<license_spring>(c.ls_config, c.activation_key);
                default:
                THROW(util::exception, "The demo license does not require a key!");
            }

        }

        switch (c.type)
        {
            case uh::licensing::config::backend_type::license_spring:
                return std::make_unique<license_spring>(c.ls_config);
            default:
                return std::make_unique<demo_license>();
        }
    }
    catch (std::exception& e){
        ERROR << "Licensing check failed for this reason: " << e.what();
        INFO << "Loading demo license.";
        return std::make_unique<demo_license>();
    }
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

bool license_package::valid()
{
    return m_backend->valid();
}

// ---------------------------------------------------------------------

} // namespace uh::licensing
