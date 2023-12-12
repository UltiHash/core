//
// Created by massi on 12/12/23.
//

#ifndef UH_CLUSTER_OWNING_SPAN_H
#define UH_CLUSTER_OWNING_SPAN_H

#include <memory>
#include <span>

namespace uh::cluster {

template<typename T>
class owning_span {
    std::size_t d_size{0};
    std::unique_ptr<T[]> data_ptr = nullptr;
public:
    owning_span() = default;

    explicit owning_span(size_t data_size) :
            d_size(data_size),
            data_ptr{std::make_unique_for_overwrite<T[]>(d_size)} {}

    owning_span(size_t data_size, std::unique_ptr<T[]> &&ptr) :
            d_size(data_size),
            data_ptr{std::move(ptr)} {}

    owning_span(owning_span &&os) noexcept: d_size(os.d_size), data_ptr(std::move(os.data_ptr)) {
        os.d_size = 0;
        os.data_ptr = nullptr;
    }

    owning_span &operator=(owning_span &&os) noexcept {
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

    inline void resize(std::size_t new_size) {
        d_size = new_size;
        data_ptr = std::make_unique_for_overwrite<T[]>(d_size);
    }

    [[nodiscard]] inline std::span<char> get_span() const noexcept {
        return {data_ptr.get(), d_size};
    }

    [[nodiscard]] inline std::string_view get_str_view() const noexcept {
        return {data_ptr.get(), d_size};
    }
};

template<typename T>
using ospan = owning_span<T>;
}
#endif //UH_CLUSTER_OWNING_SPAN_H
