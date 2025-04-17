#include <storage/group/state.h>

#include <common/utils/strings.h>
#include <stdexcept>

namespace uh::cluster::storage_group {

state state::create(std::string_view str) {
    if (str.size() < 3 || str[1] != ',') {
        throw std::runtime_error("Invalid state string format");
    }

    state result;

    auto gs = ctoul(str.front());
    if (gs >= static_cast<decltype(gs)>(group_state::SIZE)) {
        throw std::runtime_error("Group state value out of range");
    }

    result.group = static_cast<group_state>(gs);

    std::string_view storage_states_str = str.substr(2);
    result.storages.reserve(storage_states_str.size());

    for (char c : storage_states_str) {
        auto ss = ctoul(c);
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

static_assert(static_cast<int>(state::group_state::SIZE) < 10,
              "group_state has too many values");
static_assert(static_cast<int>(state::storage_state::SIZE) < 10,
              "group_state has too many values");

} // namespace uh::cluster::storage_group
