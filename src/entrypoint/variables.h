#ifndef CORE_ENTRYPOINT_VARIABLES_H
#define CORE_ENTRYPOINT_VARIABLES_H

#include <map>
#include <optional>
#include <string>

namespace uh::cluster::ep {

class variables {
public:
    typedef std::map<std::string, std::string>::const_iterator const_iterator;
    typedef std::map<std::string, std::string>::value_type value_type;

    variables() = default;
    variables(variables&&) = default;
    variables(const variables&) = default;

    variables(std::initializer_list<value_type> init);

    const_iterator begin() const;
    const_iterator end() const;

    const_iterator find(std::string_view name) const;

    std::optional<std::string_view> get(std::string_view name) const;

private:
    std::map<std::string, std::string> m_vars;
};

/**
 * Replace all occurrences of `${variable name}` with the contents of the
 * variable defined in vars. If the variable is not defined, do not replace the
 * placeholder. Occurrences of the form
 * `\${foo}` are replaced by the term `${foo}` (escaping).
 */
std::string var_replace(std::string_view format, const variables& vars);

/**
 * Compare both strings ignoring character case.
 */
bool equals_nocase(std::string_view a, std::string_view b);

/**
 * Compare string `wildcarded` with string `b`, matching `*` against
 * any particular substring and `?` against any character.
 */
bool equals_wildcard(std::string_view wildcarded, std::string_view b);

std::optional<int64_t> to_int(std::string_view s);

} // namespace uh::cluster::ep

#endif
