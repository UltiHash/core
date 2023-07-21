//
// Created by masi on 6/21/23.
//

#ifndef CORE_OSPAN_H
#define CORE_OSPAN_H

#include <memory>
#include <span>

namespace uh::cluster {

template <typename T>
struct owning_span {
    std::size_t size {};
    std::unique_ptr <T[]> data = nullptr;
    owning_span() = default;
    owning_span(size_t data_size):
            size (data_size),
            data {std::make_unique_for_overwrite <T[]> (size)} {}
    owning_span(size_t data_size, std::unique_ptr <T[]>&& ptr):
            size (data_size),
            data {std::move (ptr)} {}
    operator std::span<T>()
    {
        return std::span<T>(data.get(), size);
    }

};

template <typename T>
using ospan = owning_span <T>;

} // namespace uh::cluster

#endif //CORE_OSPAN_H
