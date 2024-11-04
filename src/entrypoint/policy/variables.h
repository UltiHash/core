#ifndef CORE_ENTRYPOINT_VARIABLES_H
#define CORE_ENTRYPOINT_VARIABLES_H

#include <common/utils/strings.h>

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
    std::map<std::string, function_type, nocase_less> m_providers;
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

private:
    const http::request& m_req;
    const command& m_cmd;
    mutable std::map<std::string, std::string, std::less<>> m_cache;
};

template <char asterisk = '*', char questionmark = '?'>
std::string var_replace(std::string_view format, const variables& vars);

/**
 * Compare string `pattern` with string `str`, matching `*` against
 * any particular substring and `?` against any character.
 * Get variables as it's input.
 * Variable replacemance on pattern is done internally.
 */
bool equals_wildcard(std::string_view pattern, std::string_view str,
                     const variables& vars);

/**
 * Compare string `pattern` with string `str`, matching `*` against
 * any particular substring and `?` against any character.
 */
template <char asterisk = '*', char questionmark = '?'>
bool equals_wildcard(std::string_view pattern, std::string_view str,
                     size_t pat_index = 0, size_t str_index = 0);

int64_t to_int(std::string_view s);

} // namespace uh::cluster::ep::policy

#endif
