#include "fragment_set_element.h"

namespace uh::cluster {
fragment_set_element::fragment_set_element(const uint128_t& ptr, uint16_t size,
                                           std::string prefix,
                                           dd::cache& storage)
    : m_storage(storage),
      m_pointer(ptr),
      m_size(size),
      m_prefix(std::move(prefix)),
      m_data(std::nullopt) {}

fragment_set_element::fragment_set_element(std::string_view data,
                                           std::string prefix,
                                           dd::cache& storage)
    : fragment_set_element(data, 0, std::move(prefix), storage) {
    m_data.emplace(data);
}

fragment_set_element::fragment_set_element(std::string_view data,
                                           const uint128_t& ptr,
                                           std::string prefix,
                                           dd::cache& storage)
    : m_storage(storage),
      m_pointer(ptr),
      m_size(std::min(static_cast<size_t>(std::numeric_limits<uint16_t>::max()),
                      data.size())),
      m_prefix(std::move(prefix)),
      m_data(std::nullopt) {}

fragment_set_element::fragment_set_element(fragment_set_element&& f) noexcept
    : m_storage(f.m_storage),
      m_pointer(f.m_pointer),
      m_size(f.m_size),
      m_prefix(std::move(f.m_prefix)),
      m_data(f.m_data) {
    f.m_size = 0;
    f.m_pointer = 0;
    f.m_data = std::nullopt;
}

void fragment_set_element::catch_frag(const fragment_set_element& f,
                                      shared_buffer<char>& data,
                                      std::string_view& str,
                                      size_t size) const {
    if (f.m_data.has_value()) {
        str = f.m_data->substr(0, size);
    } else {
        data = m_storage.read(CURRENT_CONTEXT, f.m_pointer, size);
        str = data.string_view();
    }
}

bool fragment_set_element::operator<(const fragment_set_element& f) const {
    const auto comp = m_prefix.compare(f.m_prefix);
    if (comp != 0) [[likely]] {
        return comp < 0;
    }

    const auto size = std::min(this->m_size, f.m_size);
    shared_buffer<char> d1, d2;
    std::string_view s1, s2;
    catch_frag(*this, d1, s1, size);
    catch_frag(f, d2, s2, size);
    return s1 < s2;
}

const uint128_t& fragment_set_element::pointer() const noexcept {
    return m_pointer;
}

uint16_t fragment_set_element::size() const noexcept { return m_size; }

const std::string& fragment_set_element::prefix() const noexcept {
    return m_prefix;
}

} // namespace uh::cluster
