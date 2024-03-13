#include "fragment_set_element.h"

namespace uh::cluster {
fragment_set_element::fragment_set_element(const uint128_t& ptr, uint16_t size_,
                                           const uint128_t& prefix_,
                                           global_data_view& storage)
    : m_storage(storage),
      pointer(ptr),
      size(size_),
      prefix(prefix_),
      m_data(std::nullopt) {}

fragment_set_element::fragment_set_element(const std::string_view& data,
                                           global_data_view& storage)
    : fragment_set_element(data, 0, storage) {
    m_data.emplace(data);
}

fragment_set_element::fragment_set_element(const std::string_view& data,
                                           const uint128_t& ptr,
                                           global_data_view& storage)
    : m_storage(storage),
      pointer(ptr),
      size(data.size()),
      m_data(std::nullopt) {
    std::memcpy(prefix.ref_data(), data.data(),
                std::min(sizeof(uint128_t), data.size()));
}

fragment_set_element::fragment_set_element(fragment_set_element&& f) noexcept
    : m_storage(f.m_storage),
      pointer(f.pointer),
      size(f.size),
      prefix(f.prefix),
      m_data(f.m_data) {
    f.size = 0;
    f.pointer = 0;
    f.prefix = 0;
    f.m_data = std::nullopt;
}

void fragment_set_element::catch_frag(const fragment_set_element& f,
                                      shared_buffer<char>& data,
                                      std::string_view& str, bool& l1) const {
    if (f.m_data.has_value()) {
        str = *f.m_data;
    } else if (data = m_storage.get().cached_sample(f.pointer, f.size);
               data.data() != nullptr) {
        l1 = true;
        str = data.get_str_view();
    } else {
        data = m_storage.get().read_fragment(f.pointer, f.size);
        str = data.get_str_view();
    }
}

bool fragment_set_element::operator<(const fragment_set_element& f) const {
    if (prefix != f.prefix) [[likely]] {
        return prefix < f.prefix;
    }

    shared_buffer<char> d1, d2;
    std::string_view s1, s2;
    bool b1 = false, b2 = false;

    catch_frag(*this, d1, s1, b1);
    catch_frag(f, d2, s2, b2);
    size_t comp_len = 0;

    if (b1 or b2) {
        std::string_view s1_l1 = s1;
        std::string_view s2_l1 = s2;
        if (!b1) {
            s1_l1 = s1.substr(
                0, std::min(s1.size(), m_storage.get().l1_cache_sample_size()));
        } else if (!b2) {
            s2_l1 = s2.substr(
                0, std::min(s2.size(), m_storage.get().l1_cache_sample_size()));
        }
        if (const auto comp = s1_l1.compare(s2_l1); comp != 0) {
            return comp < 0;
        }
        comp_len = std::min(s1_l1.size(), s2_l1.size());
    }

    if (b1 and !m_data.has_value() and
        size > m_storage.get().l1_cache_sample_size()) {
        d1 = m_storage.get().read_fragment(pointer, size);
        s1 = d1.get_str_view();
    }
    if (b2 and !f.m_data.has_value() and
        f.size > m_storage.get().l1_cache_sample_size()) {
        d2 = m_storage.get().read_fragment(f.pointer, f.size);
        s2 = d2.get_str_view();
    }

    return s1.substr(comp_len) < s2.substr(comp_len);
}

const uint128_t& fragment_set_element::get_pointer() const { return pointer; }

uint16_t fragment_set_element::get_size() const { return size; }

const uint128_t& fragment_set_element::get_prefix() const { return prefix; }

} // namespace uh::cluster