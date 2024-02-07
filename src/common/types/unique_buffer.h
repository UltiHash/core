#ifndef UH_CLUSTER_UNIQUE_BUFFER_H
#define UH_CLUSTER_UNIQUE_BUFFER_H

#include <memory>
#include <span>

namespace uh::cluster {

template<typename T>
class unique_buffer {
    std::size_t d_size{0};
    std::unique_ptr<T[]> data_ptr = nullptr;
public:
    constexpr unique_buffer() = default;

    constexpr explicit unique_buffer(size_t data_size) :
            d_size(data_size),
            data_ptr{std::make_unique_for_overwrite<T[]>(d_size)} {}

    constexpr unique_buffer(size_t data_size, std::unique_ptr<T[]> &&ptr) :
            d_size(data_size),
            data_ptr{std::move(ptr)} {}

    constexpr unique_buffer(const std::nullptr_t &) {}

    constexpr unique_buffer(unique_buffer &&os) noexcept: d_size(os.d_size), data_ptr(std::move(os.data_ptr)) {
        os.d_size = 0;
        os.data_ptr = nullptr;
    }

    constexpr unique_buffer &operator=(unique_buffer &&os) noexcept {
        d_size = os.d_size;
        data_ptr = std::move(os.data_ptr);
        os.d_size = 0;
        os.data_ptr = nullptr;
        return *this;
    }

    inline T *data() const noexcept {
        return data_ptr.get();
    }

    [[nodiscard]] inline constexpr size_t size() const noexcept {
        return d_size;
    }

    constexpr inline void resize(std::size_t new_size) {
        d_size = new_size;
        data_ptr = std::make_unique_for_overwrite<T[]>(d_size);
    }

    [[nodiscard]] constexpr inline std::span<char> get_span() const noexcept {
        return {data_ptr.get(), d_size};
    }

    [[nodiscard]] constexpr inline std::string_view get_str_view() const noexcept {
        return {data_ptr.get(), d_size};
    }
};
}
#endif //UH_CLUSTER_UNIQUE_BUFFER_H
