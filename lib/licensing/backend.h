#ifndef CORE_LICENSING_BACKEND_H
#define CORE_LICENSING_BACKEND_H

#include <licensing/features.h>


namespace uh::licensing
{

// ---------------------------------------------------------------------

class backend
{
public:
    virtual ~backend() = default;

    virtual bool valid() = 0;
    virtual bool has_feature(feature f) const = 0;
    virtual std::string feature_arg_string(feature f, const std::string& name) const = 0;
    virtual std::size_t feature_arg_size_t(feature f, const std::string& name) const = 0;
};

// ---------------------------------------------------------------------

} // namespace uh::licensing

#endif
