#include <common/storage_group/state.h>

#include <charconv>
#include <stdexcept>

namespace uh::cluster::storage_group {

unsigned char_to_int(const char& c) {
    unsigned ret;
    auto result = std::from_chars(&c, &c + 1, ret);

    if (result.ec != std::errc()) {
        throw std::runtime_error("Group state value out of range");
    }
    return ret;
}

state state::create(std::string_view str) {
    static_assert(static_cast<int>(group_state::SIZE) < 10,
                  "group_state has too many values");
    static_assert(static_cast<int>(storage_state::SIZE) < 10,
                  "group_state has too many values");
    if (str.size() < 3 || str[1] != ',') {
        throw std::runtime_error("Invalid state string format");
    }

    state result;

    auto gs = char_to_int(str.front());
    if (gs >= static_cast<decltype(gs)>(group_state::SIZE)) {
        throw std::runtime_error("Group state value out of range");
    }

    result.group = static_cast<group_state>(gs);

    std::string_view storage_states_str = str.substr(2);
    result.storages.reserve(storage_states_str.size());

    for (char c : storage_states_str) {
        auto ss = char_to_int(c);
        if (ss >= static_cast<int>(storage_state::SIZE)) {
            throw std::runtime_error("Group state value out of range");
        }
        result.storages.push_back(static_cast<storage_state>(ss));
    }

    return result;
}

std::string state::to_string() const {
    std::string result = std::to_string(static_cast<int>(group));
    result += ',';

    for (const auto& storage : storages) {
        result += std::to_string(static_cast<int>(storage));
    }

    return result;
}

} // namespace uh::cluster::storage_group
