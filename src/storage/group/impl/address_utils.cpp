#include "address_utils.h"

#include <common/coroutines/promise.h>
#include <common/utils/error.h>

namespace uh::cluster {

std::unordered_map<std::size_t, address_info> extract_node_address_map(
    const address& addr,
    std::function<std::pair<std::size_t, uint64_t>(uint128_t)>
        get_storage_pointer) {

    std::unordered_map<std::size_t, address_info> addr_map;
    size_t offset = 0;
    for (size_t i = 0; i < addr.size(); ++i) {
        auto frag = addr.get(i);
        const auto [id, storage_ptr] = get_storage_pointer(frag.pointer);
        const auto [last_id, last_storage_ptr] =
            get_storage_pointer(frag.pointer + frag.size - 1);

        if (id != last_id) {
            throw error_exception(
                error(error::type::unknown,
                      std::format("fragment covers multiple storages; "
                                  "storage_id_of_first_byte: {}, "
                                  "storage_id_of_last_byte: {}, "
                                  "pointer: {:X}, size: {}",
                                  id, last_id, frag.pointer, frag.size)));
        }
        frag.pointer = storage_ptr;

        auto& node_pos = addr_map[id];
        auto& node_address = node_pos.addr;
        node_address.push(frag);
        node_pos.pointer_offsets.emplace_back(offset);
        offset += frag.size;
    }

    return addr_map;
}

} // namespace uh::cluster
