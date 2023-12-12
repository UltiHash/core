//
// Created by massi on 12/12/23.
//

#ifndef UH_CLUSTER_SHARED_SPAN_H
#define UH_CLUSTER_SHARED_SPAN_H

#include <span>
#include <memory>

namespace uh::cluster {

template<typename T>
class shared_span {
    std::size_t d_size{0};
    std::shared_ptr<T[]> data_ptr = nullptr;
public:
    shared_span() = default;

    explicit shared_span(size_t data_size) :
            d_size(data_size),
            data_ptr{std::make_shared_for_overwrite<T[]>(d_size)} {}

    shared_span(size_t data_size, std::shared_ptr<T[]> &&ptr) :
            d_size(data_size),
            data_ptr{std::move(ptr)} {}

    shared_span(const std::nullptr_t &) {}

    shared_span(shared_span &&ss) noexcept: d_size(ss.d_size), data_ptr(std::move(ss.data_ptr)) {
        ss.d_size = 0;
        ss.data_ptr = nullptr;
    }

    shared_span(const shared_span &ss) noexcept: d_size(ss.d_size), data_ptr(ss.data_ptr) {
    }

    inline T *data() const noexcept {
        return data_ptr.get();
    }

    [[nodiscard]] inline constexpr size_t size() const noexcept {
        return d_size;
    }

    shared_span &operator=(shared_span &&ss) noexcept {
        d_size = ss.d_size;
        data_ptr = std::move(ss.data_ptr);
        ss.d_size = 0;
        ss.data_ptr = nullptr;
        return *this;
    }

    shared_span &operator=(const shared_span &ss) noexcept {
        d_size = ss.d_size;
        data_ptr = ss.data_ptr;
        return *this;
    }

    inline void resize(std::size_t new_size) {
        d_size = new_size;
        data_ptr = std::make_shared_for_overwrite<T[]>(d_size);
    }

    [[nodiscard]] inline std::span<char> get_span() const noexcept {
        return {data_ptr.get(), d_size};
    }

    [[nodiscard]] inline std::string_view get_str_view() const noexcept {
        return {data_ptr.get(), d_size};
    }
};

template<typename T>
using sspan = shared_span<T>;
}
#endif //UH_CLUSTER_SHARED_SPAN_H
