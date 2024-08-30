#include "address_utils.h"

namespace uh::cluster {

address_node_info
extract_node_address_map(const address& addr,
                         storage_get_handler& storage_get_handler) {
    address_node_info info;
    size_t offset = 0;
    for (size_t i = 0; i < addr.size(); ++i) {
        const auto frag = addr.get(i);
        auto n = storage_get_handler.get(frag.pointer);
        auto& node_address = info.node_address_map[n];
        if (node_address.empty()) {
            info.nodes.emplace_back(n);
        }
        node_address.push(frag);
        info.node_data_offsets_map[n].emplace_back(offset);
        offset += frag.size;
    }

    info.data_size = offset;
    return info;
}
} // namespace uh::cluster