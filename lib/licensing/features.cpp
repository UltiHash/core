#include "features.h"

#include <util/exception.h>


namespace uh::licensing
{

// ---------------------------------------------------------------------

std::string to_string(feature f)
{
    switch (f)
    {
        case feature::STORAGE: return "Storage";
        case feature::METRICS: return "Metrics";
        case feature::DEDUPLICATION: return "Deduplication";
        case feature::NETWORK_CONNECTIONS: return "NetworkConnections";
    }

    THROW(util::illegal_args, "unsupported feature type");
}

// ---------------------------------------------------------------------

feature feature_from_string(const std::string& s)
{
    if (s == "Storage")
    {
        return feature::STORAGE;
    }

    if (s == "Metrics")
    {
        return feature::METRICS;
    }

    if (s == "Deduplication")
    {
        return feature::DEDUPLICATION;
    }

    if (s == "NetworkConnections")
    {
        return feature::NETWORK_CONNECTIONS;
    }

    THROW(util::illegal_args, "unknown feature name: " + s);
}

// ---------------------------------------------------------------------

} // namespace uh::licensing
