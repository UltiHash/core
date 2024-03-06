#ifndef UH_CLUSTER_FRAGMENT_ELEMENT_H
#define UH_CLUSTER_FRAGMENT_ELEMENT_H

#include "common/global_data/global_data_view.h"

namespace uh::cluster {

class fragment_element {
    std::reference_wrapper<global_data_view> m_storage;
    uint128_t pointer{};
    uint16_t size{};
    uint128_t prefix{0};
    std::optional<std::string_view> m_data{};

public:
    fragment_element(const uint128_t& ptr, uint16_t size_,
                     const uint128_t& prefix_, global_data_view& storage);
    fragment_element(const std::string_view& data, global_data_view& storage);
    fragment_element(const std::string_view& data, const uint128_t& ptr,
                     global_data_view& storage);
    fragment_element(fragment_element&& f) noexcept;

    void catch_frag(const fragment_element& f, shared_buffer<char>& data,
                    std::string_view& str, bool& l1) const;
    bool operator<(const fragment_element& f) const;

    [[nodiscard]] const uint128_t& get_pointer() const;
    [[nodiscard]] uint16_t get_size() const;
    [[nodiscard]] const uint128_t& get_prefix() const;
};
} // namespace uh::cluster

#endif // UH_CLUSTER_FRAGMENT_ELEMENT_H
