//
// Created by masi on 5/30/23.
//

#include "span_generator.h"

namespace uh::io {

span_generator::span_generator(size_t size, std::forward_list<std::span<char>>&& spans):
    m_spans (std::move (spans)),
    m_size (size)
    {}

std::optional<data_chunk> span_generator::next() {
    if (m_spans.empty()) {
        return std::nullopt;
    }
    data_chunk dc {m_spans.front()};
    m_offset += dc.data().size ();

    m_spans.pop_front();
    return dc;
}

std::size_t span_generator::size() const {
    return m_size - m_offset;
}

} // namespace uh::io
