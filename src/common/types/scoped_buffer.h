#pragma once

#include <memory>
#include <ostream>
#include <span>

namespace uh::cluster {

template <typename T = char, bool shared = false> class scoped_buffer {
    struct info {
        info(size_t size)
            : m_size(size),
              m_capacity(size) {
            if (m_capacity > 0) {
                m_data_ptr = (T*)malloc(size * sizeof(T));
            }
        }
        ~info() {
            if (m_data_ptr != nullptr) {
                free(m_data_ptr);
            }
        }
        std::size_t m_size;
        std::size_t m_capacity;
        T* m_data_ptr = nullptr;
    };
    std::conditional_t<shared, std::shared_ptr<info>, std::unique_ptr<info>>
        m_data_info;

    static auto inline constexpr make_pointer(size_t data_size) {
        if constexpr (shared) {
            return std::make_shared<info>(data_size);
        } else {
            return std::make_unique<info>(data_size);
        }
    }

public:
    constexpr explicit scoped_buffer(size_t data_size = 0)
        : m_data_info(make_pointer(data_size)) {}

    constexpr scoped_buffer(scoped_buffer<T>&& sb) noexcept
        : m_data_info(std::move(sb.m_data_info)) {
        sb.m_data_info = make_pointer(0);
    }

    constexpr scoped_buffer(const std::nullptr_t&)
        : m_data_info(make_pointer(0)) {}

    inline T* data() const noexcept { return m_data_info->m_data_ptr; }

    inline constexpr void reserve(std::size_t size) {
        if (size > m_data_info->m_capacity) {
            m_data_info->m_data_ptr =
                (T*)realloc(m_data_info->m_data_ptr, size * sizeof(T));
            m_data_info->m_capacity = size;
        }
    }

    bool operator==(const scoped_buffer& other) const {
        return m_data_info->m_size == other.m_data_info->m_size &&
               std::equal(m_data_info->m_data_ptr,
                          m_data_info->m_data_ptr + m_data_info->m_size,
                          other.m_data_info->m_data_ptr);
    }

    inline constexpr T& operator[](size_t index) const noexcept {
        return m_data_info->m_data_ptr[index];
    }

    inline bool constexpr empty() const noexcept {
        return m_data_info->m_size == 0;
    }

    [[nodiscard]] inline constexpr size_t size() const noexcept {
        return m_data_info->m_size;
    }

    [[nodiscard]] inline constexpr size_t capacity() const noexcept {
        return m_data_info->m_capacity;
    }

    constexpr inline void resize(std::size_t new_size) {
        reserve(new_size);
        m_data_info->m_size = new_size;
    }

    [[nodiscard]] constexpr inline std::span<T> span() const noexcept {
        return {m_data_info->m_data_ptr, m_data_info->m_size};
    }

    [[nodiscard]] constexpr inline std::string_view
    string_view() const noexcept {
        return {(char*)(m_data_info->m_data_ptr),
                m_data_info->m_size * sizeof(T)};
    }

    T* begin() noexcept { return m_data_info->m_data_ptr; }
    const T* begin() const noexcept { return m_data_info->m_data_ptr; }

    T* end() noexcept { return m_data_info->m_data_ptr + m_data_info->m_size; }
    const T* end() const noexcept {
        return m_data_info->m_data_ptr + m_data_info->m_size;
    }
};

template <typename T = char> using shared_buffer = scoped_buffer<T, true>;

template <typename T = char> using unique_buffer = scoped_buffer<T, false>;

} // namespace uh::cluster

template <typename T, bool shared>
inline std::ostream&
operator<<(std::ostream& os,
           const uh::cluster::scoped_buffer<T, shared>& value) {
    for (auto i = 0ul; i < value.m_size; ++i) {
        os << value.m_data_ptr[i];
    }
    return os;
}
