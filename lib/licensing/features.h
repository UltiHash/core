#ifndef UH_LICENSING_FEATURES_H
#define UH_LICENSING_FEATURES_H

#include <string>


namespace uh::licensing
{

// ---------------------------------------------------------------------

enum class feature
{
    METRICS,
    DEDUPLICATION
};

// ---------------------------------------------------------------------

std::string to_string(feature f);
feature feature_from_string(const std::string& s);

// ---------------------------------------------------------------------

enum class metered_feature
{
    LIMIT_CPU_COUNT,
    LIMIT_STORAGE_CAPACITY,
    LIMIT_NETWORK_CONNECTIONS
};

// ---------------------------------------------------------------------

std::string to_string(metered_feature f);
metered_feature metered_from_string(const std::string& s);

// ---------------------------------------------------------------------

} // namespace uh::licensing

#endif
