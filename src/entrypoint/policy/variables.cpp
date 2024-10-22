#include "variables.h"

#include "common/telemetry/log.h"
#include "common/utils/strings.h"
#include "entrypoint/commands/command.h"

#include <charconv>
#include <iostream>
#include <stdexcept>

namespace uh::cluster::ep::policy {

namespace {

std::size_t qfind(std::string_view h, std::string_view n, std::size_t start) {
    if (n.size() > h.size()) {
        return std::string::npos;
    }

    for (auto pos = 0ull; pos <= h.size() - n.size(); ++pos) {
        std::size_t n_idx = 0ull;
        for (; n_idx < n.size(); ++n_idx) {
            if (n[n_idx] != h[pos + n_idx] && n[n_idx] != '?') {
                break;
            }
        }

        if (n_idx == n.size()) {
            return pos;
        }
    }

    return std::string::npos;
}

value_provider make_value_provider() {
    value_provider vp;

    vp.add(variables::NAME_ACTION_ID,
           [](const auto&, const auto& c) { return c.action_id(); });

    vp.add(variables::NAME_RESOURCE_ARN, [](const auto& r, const auto&) {
        return "arn:aws:s3:::" + r.bucket() + "/" + r.object_key();
    });

    vp.add(variables::NAME_PRINCIPAL, [](const auto& r, const auto&) {
        return r.authenticated_user().arn;
    });

    vp.add("aws:username", [](const auto& r, const auto&) {
        return r.authenticated_user().name;
    });

    vp.add("aws:userid", [](const auto& r, const auto&) {
        return r.authenticated_user().id;
    });

    vp.add("aws:SourceIp", [](const auto& r, const auto&) {
        return r.peer().address().to_string();
    });

    vp.add("aws:referer",
           [](const auto& r, const auto&) { return r.header("Referer"); });

    vp.add("aws:UserAgent",
           [](const auto& r, const auto&) { return r.header("User-Agent"); });

    vp.add("s3:x-amz-content-sha256", [](const auto& r, const auto&) {
        return r.header("x-amz-content-sha256");
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
                    }

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

bool equals_nocase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }

    for (auto i = 0ull; i < a.size(); ++i) {
        if (tolower(a[i]) != tolower(b[i])) {
            return false;
        }
    }

    return true;
}

bool equals_wildcard(std::string_view wildcarded, std::string_view b) {
    if (wildcarded.empty()) {
        return b.empty();
    }

    auto groups = split(wildcarded, '*');
    if (groups.size() == 1) {
        return qfind(b, groups[0], 0) == 0;
    }

    std::size_t pos = 0;
    for (const auto& g : groups) {
        pos = qfind(b, g, pos);

        if (pos == std::string_view::npos) {
            return false;
        }

        pos += g.size();
    }

    return true;
}

std::optional<int64_t> to_int(std::string_view s) {
    int64_t rv;

    auto result = std::from_chars(s.begin(), s.end(), rv);
    if (result.ptr != s.end() || result.ec != std::errc()) {
        return {};
    }

    return rv;
}
} // namespace uh::cluster::ep::policy
