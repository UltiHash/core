#include "address.h"

#include <common/utils/pointer_traits.h>

#include <format>

namespace uh::cluster {

template <> std::string fragment_t<pointer>::to_string() const {
    return std::format("[group {}, pointer {:016x}, size {:x}]",
                       pointer_traits::get_group_id(pointer),
                       pointer_traits::get_group_pointer(pointer), size);
}

template <> std::string fragment_t<storage_pointer>::to_string() const {
    return std::format("[pointer {:016x}, size {:x}]", pointer, size);
}

} // namespace uh::cluster
