//
// Created by masi on 5/30/23.
//

#ifndef CORE_SPAN_GENERATOR_H
#define CORE_SPAN_GENERATOR_H

#include <span>
#include <forward_list>

#include <io/data_generator.h>

namespace uh::io {

class span_generator: public data_generator {

    public:

        explicit span_generator (size_t size, std::forward_list <std::span <char>>&& spans);

        inline std::optional<data_chunk> next() override;

        [[nodiscard]] std::size_t size() const override;

    private:
        std::forward_list <std::span <char>> m_spans{};
        const size_t m_size;
        size_t m_offset {};
};

} // namespace uh::io


#endif //CORE_SPAN_GENERATOR_H
