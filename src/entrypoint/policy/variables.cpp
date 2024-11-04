#include "variables.h"
#include "parser.h"

#include "common/telemetry/log.h"
#include "common/utils/strings.h"
#include "entrypoint/commands/command.h"
#include "entrypoint/formats.h"

#include <algorithm>
#include <charconv>
#include <iostream>
#include <stdexcept>

namespace uh::cluster::ep::policy {

namespace {

/*
 * Only alphanumeric characters and the following characters are allowed in IAM
 * paths: forward slash (/), plus (+), equals (=), comma (,), period (.),
 * at (@), underscore (_), and hyphen (-).
 * https://docs.aws.amazon.com/IAM/latest/UserGuide/reference-arns.html#arns-paths
 */
value_provider make_value_provider() {
    value_provider vp;

    vp.add("uh:ActionId", [](const http::request& r, const command& c) {
        return c.action_id();
    });

    vp.add("uh:ResourceArn", [](const http::request& r, const command& c) {
        return "arn:aws:s3:::" + r.bucket() + "/" + r.object_key();
    });

    vp.add("aws:PrincipalArn", [](const http::request& r, const command& c) {
        return r.authenticated_user().arn;
    });

    vp.add("aws:username", [](const http::request& r, const command& c) {
        return r.authenticated_user().name;
    });

    vp.add("aws:userid", [](const http::request& r, const command& c) {
        return r.authenticated_user().id;
    });

    vp.add("aws:SourceIp", [](const http::request& r, const command& c) {
        return r.peer().address().to_string();
    });

    vp.add("aws:referer", [](const http::request& r, const command& c) {
        return r.header("Referer");
    });

    vp.add("aws:UserAgent", [](const http::request& r, const command& c) {
        return r.header("User-Agent");
    });

    vp.add("s3:x-amz-content-sha256",
           [](const http::request& r, const command& c) {
               return r.header("x-amz-content-sha256");
           });

    vp.add("s3:x-amz-copy-source",
           [](const http::request& r, const command& c) {
               return r.header("x-amz-copy-source");
           });

    vp.add("s3:delimiter", [](const http::request& r, const command& c) {
        return r.query("delimiter");
    });

    vp.add("s3:prefix", [](const http::request& r, const command& c) {
        return r.query("prefix");
    });

    vp.add("aws:CurrentTime", [](const http::request& r, const command& c) {
        return iso8601_date(std::chrono::system_clock::now());
    });

    return vp;
}

value_provider& default_value_provider() {
    static value_provider vp = make_value_provider();
    return vp;
}

} // namespace

std::optional<std::string> value_provider::get(std::string_view name,
                                               const http::request& r,
                                               const command& c) const {
    auto it = m_providers.find(name);
    if (it == m_providers.end()) {
        return {};
    }

    return it->second(r, c);
}

void value_provider::add(const std::string& name, function_type func) {
    m_providers[name] = std::move(func);
}

variables::variables(const http::request& req, const command& cmd)
    : m_req(req),
      m_cmd(cmd) {}

std::optional<std::string_view> variables::get(std::string_view name) const {
    if (auto it = m_cache.find(name); it != m_cache.end()) {
        return it->second;
    }

    if (auto value = default_value_provider().get(name, m_req, m_cmd); value) {
        auto res = m_cache.emplace(std::move(name), std::move(*value));
        return res.first->second;
    }

    return {};
}

void variables::put(std::string k, std::string v) {
    m_cache[std::move(k)] = std::move(v);
}

// https://docs.aws.amazon.com/IAM/latest/UserGuide/reference_policies_variables.html
template <char asterisk, char questionmark>
std::string var_replace(std::string_view format, const variables& vars) {
    std::string rv;
    char last = 0;

    for (std::size_t i = 0; i < format.size(); ++i) {
        auto current = format[i];
        if (last == '\\') {
            rv.append(1, current);
            last = 0;
            continue;
        }

        switch (current) {
        case '$':
            if (i + 1 >= format.size()) {
                rv.append(1, current);
                continue;
            }

            if (format[i + 1] == '{') {
                auto end_of_var = format.find('}', i + 1);
                if (end_of_var != std::string::npos) {
                    auto var_name = format.substr(i + 2, end_of_var - i - 2);

                    if (auto it = vars.get(var_name); it) {
                        rv.append(*it);
                    } else {
                        if (var_name.size() == 1) {
                            if (var_name[0] == asterisk)
                                rv.push_back('*');
                            if (var_name[0] == questionmark)
                                rv.push_back('?');
                            if (var_name[0] == '$')
                                rv.push_back('$');
                        }
                    }

                    // vp.add("*", [](const auto& r, const auto&) { return
                    // std::string("*"); }); vp.add("?", [](const auto& r, const
                    // auto&) { return std::string("?"); }); vp.add("$",
                    // [](const auto& r, const auto&) { return std::string("$");
                    // });
                    //

                    i = end_of_var;
                } else {
                    rv.append(format.substr(i));
                    i = format.size();
                }
            }
            break;
        case '\\':
            break;

        default:
            rv.append(1, current);
            break;
        }

        last = current;
    }

    return rv;
}

template std::string var_replace<'*', '?'>(std::string_view format,
                                           const variables& vars);

std::string remap_wildcards(std::string& str) { return str; }

std::string remap_wildcards(std::string&& str) {
    std::string result = std::move(str);
    return remap_wildcards(result);
}

// https://docs.aws.amazon.com/IAM/latest/UserGuide/reference_policies_elements_resource.html

template <char asterisk, char questionmark>
bool equals_wildcard(std::string_view pattern, std::string_view str,
                     size_t pat_index, size_t str_index) {
    if (pat_index == 0 && str_index == 0) {
        std::cout << "pattern: " << pattern << std::endl;
        std::cout << "str: " << str << std::endl;
    }
    if (pat_index == pattern.size()) {
        return str_index == str.size();
    }

    if (pattern[pat_index] == asterisk) {
        return equals_wildcard<asterisk, questionmark>( //
                   pattern, str, pat_index + 1, str_index) ||
               (str_index < str.size() &&
                equals_wildcard<asterisk, questionmark>( //
                    pattern, str, pat_index, str_index + 1));
    }

    if ((pattern[pat_index] == questionmark) ||
        (str_index < str.size() && str[str_index] == pattern[pat_index])) {
        return equals_wildcard<asterisk, questionmark>(
            pattern, str, pat_index + 1, str_index + 1);
    }

    return false;
}

template bool equals_wildcard<'*', '?'>(std::string_view pattern,
                                        std::string_view str, size_t pat_index,
                                        size_t str_index);

bool equals_wildcard(std::string_view pattern, std::string_view str,
                     const variables& vars) {
    constexpr char temp_asterisk = -1;
    constexpr char temp_questionmark = -2;

    // Replace wildcard characters.
    static std::unordered_map<char, char> replacements = {
        {'*', temp_asterisk}, {'?', temp_questionmark}};
    auto buf = std::string();
    buf.reserve(pattern.size());
    std::transform(
        pattern.begin(), pattern.end(), std::back_inserter(buf),
        [&](char c) { return replacements.count(c) ? replacements[c] : c; });

    // Evaluate variables, including special character variables.
    var_replace<temp_asterisk, temp_questionmark>(buf, vars);

    // Do wildcard matching with replaced wildcard characters
    return equals_wildcard<temp_asterisk, temp_questionmark>(buf, str);
}

int64_t to_int(std::string_view s) {
    int64_t rv;

    auto result = std::from_chars(s.begin(), s.end(), rv);
    if (result.ptr != s.end() || result.ec != std::errc()) {
        throw std::runtime_error("string to int conversion failed");
    }

    return rv;
}

} // namespace uh::cluster::ep::policy
