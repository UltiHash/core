#pragma once

#include <storage/global/cache.h>
#include <storage/global/data_view.h>

namespace uh::cluster {
class fragment_set_element {
public:
    /**
     * Constructor used to re-construct fragment_set_elements during the replay
     * operation of the fragment_set_log
     * @param ptr Content of the pointer member
     * @param size_ Content of the size member
     * @param prefix Content of the prefix member
     * @param storage  A reference to the global_data_view to be used.
     */
    fragment_set_element(const uint128_t& ptr, uint16_t size_,
                         std::string prefix, storage::global::cache& storage);

    /**
     * Creates a fragment_set_element that holds the full fragment data, used
     * for the parameter to the #find method in the fragment_set
     * @param data The full content of the fragment
     * @param prefix the prefix of data to be stored in set
     * @param storage A reference to the global_data_view to find similar
     * fragments in.
     */
    fragment_set_element(std::string_view data, std::string prefix,
                         storage::global::cache& storage);
    /**
     * Creates a fragment_set_element that holds only the prefix and the pointer
     * @param data The full content of the fragment, where only the prefix of 16
     * bytes is kept of.
     * @param ptr The pointer to the full fragment.
     * @param prefix the prefix of data to be stored in set
     * @param storage A reference to the global_data_view the full fragment
     * resides in.
     */
    fragment_set_element(std::string_view data, const uint128_t& ptr,
                         std::string prefix, storage::global::cache& storage);

    /**
     * Move-constructs a fragment_set_element
     * @param f rvalue reference to the instance to be moved
     */
    fragment_set_element(fragment_set_element&& f) noexcept;

    /**
     * @brief Provides ordering information for fragment_set_element instances
     * @param f A constant reference to an fragment_set_element to be compared
     * against
     * @return true if this fragment_set_element is lexicographically smaller
     * than #f, false otherwise.
     */
    bool operator<(const fragment_set_element& f) const;

    [[nodiscard]] const uint128_t& pointer() const noexcept;

    [[nodiscard]] uint16_t size() const noexcept;

    [[nodiscard]] const std::string& prefix() const noexcept;

    mutable std::atomic<int> m_hint_count = 0;

private:
    friend std::ostream& operator<<(std::ostream& os,
                                    const fragment_set_element& value);
    storage::global::cache& m_storage;
    uint128_t m_pointer{};
    uint16_t m_size{};
    std::string m_prefix;
    std::optional<std::string_view> m_data{};
    void catch_frag(const fragment_set_element& f, shared_buffer<char>& data,
                    std::string_view& str, size_t size) const;
};

inline std::ostream& operator<<(std::ostream& os,
                                const fragment_set_element& value) {
    shared_buffer<char> sb;
    std::string_view sv;
    value.catch_frag(value, sb, sv, value.m_size);
    os << sv;
    return os;
}

} // namespace uh::cluster
