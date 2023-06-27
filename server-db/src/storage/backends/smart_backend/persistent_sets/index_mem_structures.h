//
// Created by masi on 6/8/23.
//

#ifndef CORE_INDEX_MEM_STRUCTURES_H
#define CORE_INDEX_MEM_STRUCTURES_H

#include "set_interface.h"
#include <stdexcept>

namespace uh::dbn::storage::smart::sets {


enum color_t : uint8_t
{
    RED = 0,
    BLACK = 1
};

struct offset_span {
    uint64_t m_data_offset;
    size_t m_size;
};

struct mmap_node {
    offset_span m_data;
    uint64_t m_parent;
    uint64_t m_left;
    uint64_t m_right;
    color_t m_color;
    uint64_t data_prefix = 0;
};
struct node {
    uint64_t m_offset;
    mmap_node* m_mnode;
};

enum direction_t: uint8_t {
    LEFT = 0,
    RIGHT = 1,
};

struct alignas (4096) block {

    static constexpr auto nodes_space = 4096 - sizeof (uint8_t);
    static constexpr auto nodes_count = nodes_space / sizeof (mmap_node);
    static constexpr auto effective_node_space = nodes_count * sizeof (mmap_node);

    inline node acquire_node () {
        if (full ()) {
            throw std::overflow_error ("no empty nodes to be acquired");
        }
        const auto offset = offsetof (block, nodes) + empty_node_id * sizeof (mmap_node);
        return {offset, &nodes [empty_node_id++]};
    }

    [[nodiscard]] inline bool full () const {
        return empty_node_id == nodes_count;
    }

    uint8_t empty_node_id {0};
    mmap_node nodes [nodes_count] {};
};

struct alignas (4096) first_block {
    static constexpr auto nodes_space = 4096 - 5 * sizeof (size_t) - sizeof (uint8_t);
    static constexpr auto nodes_count = nodes_space / sizeof (mmap_node);
    static constexpr auto effective_node_space = nodes_count * sizeof (mmap_node);

    inline node acquire_node () {
        if (full ()) {
            throw std::overflow_error ("no empty nodes to be acquired");
        }
        const auto offset = offsetof (first_block, nodes) + empty_node_id * sizeof (mmap_node);
        return {offset, &nodes [empty_node_id++]};
    }

    [[nodiscard]] inline bool full () const {
        return empty_node_id == nodes_count;
    }
    std::size_t empty_hole_size {0};
    uint64_t mix_block_offset {0};
    uint64_t root_offset {0};
    uint64_t empty_block {0};
    uint64_t nill_offset {0};

    uint8_t empty_node_id {0};
    mmap_node nodes [nodes_count] {};
};
} // end namespace uh::dbn::storage::smart::sets

#endif //CORE_INDEX_MEM_STRUCTURES_H
