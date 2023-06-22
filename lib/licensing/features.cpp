#include "features.h"

#include <util/exception.h>


namespace uh::licensing
{

// ---------------------------------------------------------------------

std::string to_string(feature f)
{
    switch (f)
    {
        case feature::METRICS: return "METRICS";
        case feature::DEDUPLICATION: return "DEDUPLICATION";
    }

    THROW(util::illegal_args, "unsupported feature type");
}

// ---------------------------------------------------------------------

feature feature_from_string(const std::string& s)
{
    if (s == "METRICS")
    {
        return feature::METRICS;
    }

    if (s == "DEDUPLICATION")
    {
        return feature::DEDUPLICATION;
    }

    THROW(util::illegal_args, "unknown feature name: " + s);
}

// ---------------------------------------------------------------------

std::string to_string(metered_feature f)
{
    switch (f)
    {
        case metered_feature::LIMIT_CPU_COUNT: return "LIMIT_CPU_COUNT";
        case metered_feature::LIMIT_STORAGE_CAPACITY: return "LIMIT_STORAGE_CAPACITY";
        case metered_feature::LIMIT_NETWORK_CONNECTIONS: return "LIMIT_NETWORK_CONNECTIONS";
    }

    THROW(util::illegal_args, "unsupported metered feature type");
}

// ---------------------------------------------------------------------

metered_feature metered_from_string(const std::string& s)
{
    if (s == "LIMIT_CPU_COUNT")
    {
        return metered_feature::LIMIT_CPU_COUNT;
    }

    if (s == "LIMIT_STORAGE_CAPACITY")
    {
        return metered_feature::LIMIT_STORAGE_CAPACITY;
    }

    if (s == "LIMIT_NETWORK_CONNECTIONS")
    {
        return metered_feature::LIMIT_NETWORK_CONNECTIONS;
    }

    THROW(util::illegal_args, "unknown metered feature name: " + s);
}


// ---------------------------------------------------------------------

} // namespace uh::licensing
