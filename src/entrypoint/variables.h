#ifndef CORE_ENTRYPOINT_VARIABLES_H
#define CORE_ENTRYPOINT_VARIABLES_H

#include <map>
#include <optional>
#include <string>

namespace uh::cluster::ep {

class variables {
public:
    typedef std::map<std::string, std::string>::const_iterator const_iterator;

    const_iterator begin() const;
    const_iterator end() const;

    const_iterator find(std::string_view name) const;

    std::optional<std::string_view> get(std::string_view name) const;

private:
    std::map<std::string, value> m_vars;
};

} // namespace uh::cluster::ep

#endif
