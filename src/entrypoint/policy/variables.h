#ifndef CORE_ENTRYPOINT_VARIABLES_H
#define CORE_ENTRYPOINT_VARIABLES_H

#include <functional>
#include <map>
#include <optional>
#include <string>

namespace uh::cluster {

class command;

namespace ep::http {

class request;

}

} // namespace uh::cluster

namespace uh::cluster::ep::policy {

class value_provider {
public:
    typedef std::function<std::optional<std::string>(const http::request&,
                                                     const command&)>
        function_type;

    std::optional<std::string>
    get(std::string_view name, const http::request& r, const command& c) const;

    void add(const std::string& name, function_type func);

private:
    std::map<std::string, function_type, std::less<>> m_providers;
};

class variables {
public:
    typedef std::map<std::string, std::string>::const_iterator const_iterator;
    typedef std::map<std::string, std::string>::value_type value_type;

    variables(const http::request& req, const command& cmd);
    variables(variables&&) = default;
    variables(const variables&) = default;

    std::optional<std::string_view> get(std::string_view name) const;
    void put(std::string k, std::string v);

    static constexpr const char* NAME_ACTION_ID = "uh:ActionId";
    static constexpr const char* NAME_RESOURCE_ARN = "uh:ResourceArn";
    static constexpr const char* NAME_PRINCIPAL = "aws:PrincipalArn";

private:
    const http::request& m_req;
    const command& m_cmd;
    mutable std::map<std::string, std::string, std::less<>> m_cache;
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

} // namespace uh::cluster::ep::policy

#endif
