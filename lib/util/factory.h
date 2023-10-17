#ifndef UTILS_FACTORY_H
#define UTILS_FACTORY_H

#include <memory>


namespace uh::util
{

// ---------------------------------------------------------------------

template <typename type>
class factory
{
public:
    virtual ~factory() = default;

    virtual std::unique_ptr<type> create() = 0;
};

// ---------------------------------------------------------------------

} // namespace uh::util

#endif
