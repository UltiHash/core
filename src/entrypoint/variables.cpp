#include "variables.h"

#include <iostream>

namespace uh::cluster::ep {

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

} // namespace uh::cluster::ep
