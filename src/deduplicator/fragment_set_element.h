#ifndef UH_CLUSTER_FRAGMENT_SET_ELEMENT_H
#define UH_CLUSTER_FRAGMENT_SET_ELEMENT_H

#include "common/global_data/global_data_view.h"

namespace uh::cluster {

class fragment_set_element {
    std::reference_wrapper<global_data_view> m_storage;
    uint128_t m_pointer{};
    uint16_t m_size{};
    uint128_t m_prefix{0};
    std::optional<std::string_view> m_data{};

public:
    fragment_set_element(const uint128_t& ptr, uint16_t size_,
                         const uint128_t& prefix_, global_data_view& storage);
    fragment_set_element(const std::string_view& data,
                         global_data_view& storage);
    fragment_set_element(const std::string_view& data, const uint128_t& ptr,
                         global_data_view& storage);
    fragment_set_element(fragment_set_element&& f) noexcept;

    void catch_frag(const fragment_set_element& f, shared_buffer<char>& data,
                    std::string_view& str, bool& l1) const;
    bool operator<(const fragment_set_element& f) const;

    [[nodiscard]] const uint128_t& pointer() const;
    [[nodiscard]] uint16_t size() const;
    [[nodiscard]] const uint128_t& prefix() const;
};
} // namespace uh::cluster

#endif // UH_CLUSTER_FRAGMENT_SET_ELEMENT_H
