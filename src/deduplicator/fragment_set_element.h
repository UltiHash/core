#ifndef UH_CLUSTER_FRAGMENT_SET_ELEMENT_H
#define UH_CLUSTER_FRAGMENT_SET_ELEMENT_H

#include "common/global_data/global_data_view.h"

namespace uh::cluster {

class fragment_set_element {
public:
    fragment_set_element(const uint128_t& ptr, uint16_t size_,
                         const uint128_t& prefix_, global_data_view& storage);
    fragment_set_element(const std::string_view& data,
                         global_data_view& storage);
    fragment_set_element(const std::string_view& data, const uint128_t& ptr,
                         global_data_view& storage);
    fragment_set_element(fragment_set_element&& f) noexcept;

    /**
     * @brief Provides ordering information for fragment_set_element instances
     * @param f A constant reference to an fragment_set_element to be compared
     * against
     * @return true if this fragment_set_element is lexicographically smaller
     * than #f, false otherwise.
     */
    bool operator<(const fragment_set_element& f) const;

    [[nodiscard]] const uint128_t& pointer() const;
    [[nodiscard]] uint16_t size() const;
    [[nodiscard]] const uint128_t& prefix() const;

private:
    std::reference_wrapper<global_data_view> m_storage;
    uint128_t m_pointer{};
    uint16_t m_size{};
    uint128_t m_prefix{0};
    std::optional<std::string_view> m_data{};

    void catch_frag(const fragment_set_element& f, shared_buffer<char>& data,
                    std::string_view& str, bool& l1) const;
};
} // namespace uh::cluster

#endif // UH_CLUSTER_FRAGMENT_SET_ELEMENT_H
