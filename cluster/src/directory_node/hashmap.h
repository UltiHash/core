//
// Created by masi on 9/25/23.
//

#ifndef CORE_HASHMAP_H
#define CORE_HASHMAP_H

#include <functional>

namespace uh::cluster {

class direct_handler {};
class growing_managed_storage_handler {};

template <typename KeyHandle, typename ValueHandle>
requires std::is_move_constructible_v <KeyHandle>
        and std::is_move_constructible_v <ValueHandle>
        and (not std::is_copy_constructible_v <KeyHandle>)
        and (not std::is_copy_constructible_v <ValueHandle>)
class hashmap {
public:
    hashmap (KeyHandle key_handle, ValueHandle value_handle):
        m_key_handle (std::move (key_handle)), m_value_handle (std::move (value_handle)) {}


private:

    // plain storage for hash index
    
    KeyHandle m_key_handle;
    ValueHandle m_value_handle;
};

} // end namespace uh::cluster

#endif //CORE_HASHMAP_H
