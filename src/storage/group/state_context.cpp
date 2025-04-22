#include <storage/group/state_context.h>

#include <common/utils/strings.h>
#include <stdexcept>

namespace uh::cluster::storage::group {

state_context state_context::create(std::string_view str) {
    if (str.size() < 3 || str[1] != ',') {
        throw std::runtime_error("Invalid state_context string format");
    }

    state_context result;

    auto gs = ctoul(str.front());
    if (gs >= magic_enum::enum_count<group_state>()) {
        throw std::runtime_error("Group state value out of range");
    }

    result.group = static_cast<group_state>(gs);

    std::string_view storage_states_str = str.substr(2);
    result.storages.reserve(storage_states_str.size());

    for (char c : storage_states_str) {
        auto ss = ctoul(c);
        if (ss >= magic_enum::enum_count<storage_state>()) {
            throw std::runtime_error("Group state value out of range");
        }
        result.storages.push_back(static_cast<storage_state>(ss));
    }

    return result;
}

std::string state_context::to_string() const {
    std::string result = std::to_string(static_cast<int>(group));
    result += ',';

    for (const auto& storage : storages) {
        result += std::to_string(static_cast<int>(storage));
    }

    return result;
}

static_assert(magic_enum::enum_count<group_state>() < 10,
              "group_state has too many values");
static_assert(magic_enum::enum_count<storage_state>() < 10,
              "storage_state has too many values");

} // namespace uh::cluster::storage::group
