#include "variables.h"

#include "common/utils/strings.h"

#include <charconv>
#include <iostream>
#include <stdexcept>

namespace uh::cluster::ep {

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

} // namespace

variables::variables(std::initializer_list<variables::value_type> init)
    : m_vars(std::move(init)) {}

variables::const_iterator variables::begin() const { return m_vars.begin(); }

variables::const_iterator variables::end() const { return m_vars.end(); }

variables::const_iterator variables::find(std::string_view name) const {
    return m_vars.find(std::string(name));
}

std::optional<std::string_view> variables::get(std::string_view name) const {
    auto it = find(name);
    if (it == end()) {
        return {};
    }

    return it->second;
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

                    auto it = vars.find(var_name);
                    if (it != vars.end()) {
                        rv.append(it->second);
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
} // namespace uh::cluster::ep
