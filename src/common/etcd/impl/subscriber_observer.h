#pragma once

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/utils/strings.h>
#include <type_traits>

namespace uh::cluster {

struct subscriber_observer {
    virtual bool on_watch(etcd_manager::response resp) = 0;
    virtual ~subscriber_observer() = default;
};

template <typename T>
concept Serializable = requires(const T& t, const std::string& s) {
    { serialize(t) } -> std::convertible_to<std::string>;
    { deserialize<T>(s) } -> std::same_as<T>;
};

} // namespace uh::cluster
