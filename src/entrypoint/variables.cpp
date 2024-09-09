#include "variables.h"

namespace uh::cluster::ep {

variables::const_iterator variables::begin() const { return m_vars.begin(); }

variables::const_iterator variables::end() const { return m_vars.end(); }

variables::const_iterator variables::find(std::string_view name) const {
    return m_vars.find(name);
}

std::optional<std::string_view> variables::get(std::string_view name) const {
    auto it = find(name);
    if (name == end()) {
        return {};
    }

    return it->second;
}

} // namespace uh::cluster::ep
