#ifndef UH_CLUSTER_SHARED_BUFFER_H
#define UH_CLUSTER_SHARED_BUFFER_H

#include <memory>
#include <span>

namespace uh::cluster {

template <typename T> class shared_buffer {
    std::size_t d_size{0};
    std::shared_ptr<T[]> data_ptr = nullptr;

public:
    constexpr shared_buffer() = default;

    constexpr explicit shared_buffer(size_t data_size)
        : d_size(data_size),
          data_ptr{std::make_shared_for_overwrite<T[]>(d_size)} {}

    constexpr shared_buffer(size_t data_size, std::shared_ptr<T[]>&& ptr)
        : d_size(data_size),
          data_ptr{std::move(ptr)} {}

    constexpr shared_buffer(const std::nullptr_t&) {}

    constexpr shared_buffer(shared_buffer&& ss) noexcept
        : d_size(ss.d_size),
          data_ptr(std::move(ss.data_ptr)) {
        ss.d_size = 0;
        ss.data_ptr = nullptr;
    }

    constexpr shared_buffer(const shared_buffer& ss) noexcept
        : d_size(ss.d_size),
          data_ptr(ss.data_ptr) {}

    inline T* data() const noexcept { return data_ptr.get(); }

    [[nodiscard]] inline constexpr size_t size() const noexcept {
        return d_size;
    }

    constexpr shared_buffer& operator=(shared_buffer&& ss) noexcept {
        d_size = ss.d_size;
        data_ptr = std::move(ss.data_ptr);
        ss.d_size = 0;
        ss.data_ptr = nullptr;
        return *this;
    }

    constexpr shared_buffer& operator=(const shared_buffer& ss) noexcept {
        d_size = ss.d_size;
        data_ptr = ss.data_ptr;
        return *this;
    }

    constexpr inline void resize(std::size_t new_size) {
        d_size = new_size;
        data_ptr = std::make_shared_for_overwrite<T[]>(d_size);
    }

    [[nodiscard]] constexpr inline std::span<char> get_span() const noexcept {
        return {data_ptr.get(), d_size};
    }

    [[nodiscard]] constexpr inline std::string_view
    get_str_view() const noexcept {
        return {data_ptr.get(), d_size};
    }
};

} // namespace uh::cluster
#endif // UH_CLUSTER_SHARED_BUFFER_H
