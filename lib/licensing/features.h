#ifndef UH_LICENSING_FEATURES_H
#define UH_LICENSING_FEATURES_H

#include <string>


namespace uh::licensing
{

// ---------------------------------------------------------------------

enum class feature
{
    STORAGE,
    METRICS,
    DEDUPLICATION,
    NETWORK_CONNECTIONS
};

// ---------------------------------------------------------------------

std::string to_string(feature f);
feature feature_from_string(const std::string& s);

// ---------------------------------------------------------------------

} // namespace uh::licensing

#endif
