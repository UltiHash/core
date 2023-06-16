//
// Created by masi on 6/8/23.
//

#ifndef CORE_BLOCKED_MEMORY_H
#define CORE_BLOCKED_MEMORY_H

#include "fragment_set_interface.h"

namespace uh::dbn::storage::smart::sets {


enum color_t : uint8_t
{
    RED = 0,
    BLACK = 1
};

struct mmap_node {
    fragment m_frag;
    uint64_t m_parent;
    uint64_t m_left;
    uint64_t m_right;
    color_t m_color;
};
struct node {
    uint64_t m_offset;
    mmap_node* m_mnode;
};


struct alignas (4096) block {

    static constexpr auto nodes_space = 4096 - sizeof (uint8_t);
    static constexpr auto nodes_count = nodes_space / sizeof (mmap_node);
    static constexpr auto effective_node_space = nodes_count * sizeof (mmap_node);


    inline std::pair <uint8_t, mmap_node*> acquire_mnode () {
        if (full ()) {
            throw std::exception ();
        }
        return {empty_node_id, &nodes [empty_node_id++]};
    }
    inline node acquire_node () {
        if (full ()) {
            throw std::exception ();
        }
        const auto offset = offsetof (block, nodes) + empty_node_id * sizeof (mmap_node);
        return {offset, &nodes [empty_node_id++]};
    }

    inline mmap_node* get_node (uint8_t id) noexcept {
        return &nodes[id];
    }

    [[nodiscard]] inline bool full () const {
        return empty_node_id == nodes_count;
    }

    uint8_t empty_node_id {};
    mmap_node nodes [nodes_count]{};
};

struct alignas (4096) first_block {
    static constexpr auto nodes_space = 4096 - 5 * sizeof (size_t) - sizeof (uint8_t);
    static constexpr auto nodes_count = nodes_space / sizeof (mmap_node);
    static constexpr auto effective_node_space = nodes_count * sizeof (mmap_node);

    inline node acquire_node () {
        if (full ()) {
            throw std::exception ();
        }
        const auto offset = offsetof (first_block, nodes) + empty_node_id * sizeof (mmap_node);
        return {offset, &nodes [empty_node_id++]};
    }
    inline std::pair <uint8_t, mmap_node*> acquire_mnode () {
        if (full ()) {
            throw std::exception ();
        }
        return {empty_node_id, &nodes [empty_node_id++]};
    }
    inline mmap_node* get_node (uint8_t id) noexcept {
        return &nodes[id];
    }
    [[nodiscard]] inline bool full () const {
        return empty_node_id == nodes_count;
    }
    std::size_t empty_hole_size {};
    uint64_t mix_block_offset {};
    uint64_t root_offset {};
    uint64_t empty_block {};
    uint64_t nill_offset {};

    uint8_t empty_node_id {};
    mmap_node nodes [nodes_count]{};
};

struct blocked_memory {
    explicit blocked_memory(void* memory, size_t max_empty_hole_size): blocks (reinterpret_cast <block*> (memory)),
                                                                       m_first_block (*(reinterpret_cast <first_block*> (memory))),
                                                                       m_max_empty_hole_size (max_empty_hole_size){}

    [[nodiscard]] inline node get_node (uint64_t address) const {
        const auto block_id = address >> 8;
        const uint8_t node_id = address & 0x0f;
        if (block_id == 0) {
            return {address, m_first_block.get_node(node_id)};
        }
        else {
            return {address, blocks[block_id].get_node(node_id)};
        }
    }

    inline node add_node (uint64_t parent) {

        static auto insert = [this] (auto& b, unsigned long block_id) {
            node n;

            if (!b.full()) {
                const auto ac_n = b.acquire_mnode();
                n.m_mnode = ac_n.second;
                n.m_offset = block_id << 8;
                n.m_offset |= ac_n.first;
                m_first_block.empty_hole_size -= sizeof (mmap_node);
            }
            else if (m_first_block.empty_hole_size < m_max_empty_hole_size) {
                const auto ac_n = blocks [m_first_block.empty_block].acquire_mnode();
                n.m_mnode = ac_n.second;
                n.m_offset = m_first_block.empty_block << 8;
                n.m_offset |= ac_n.first;
                m_first_block.empty_block += 1;
                m_first_block.empty_hole_size -= sizeof (mmap_node);
            }
            else if (!blocks [m_first_block.mix_block_offset].full()) {
                const auto ac_n = blocks [m_first_block.mix_block_offset].acquire_mnode();
                n.m_mnode = ac_n.second;
                n.m_offset = m_first_block.mix_block_offset << 8;
                n.m_offset |= ac_n.first;
                m_first_block.empty_hole_size -= sizeof (mmap_node);
            }
            else {
                const auto ac_n = blocks [m_first_block.empty_block].acquire_mnode();
                n.m_mnode = ac_n.second;
                n.m_offset = m_first_block.empty_block << 8;
                n.m_offset |= ac_n.first;
                m_first_block.mix_block_offset = m_first_block.empty_block;
                m_first_block.empty_block += 1;
                m_first_block.empty_hole_size -= sizeof (mmap_node);
            }
            return n;
        };

        const auto block_id = parent >> 8;
        if (block_id == 0) {
            return insert (m_first_block, block_id);
        }
        else {
            return insert (blocks [block_id], block_id);
        }
    }

    first_block& m_first_block;
    block* blocks;
    const size_t m_max_empty_hole_size;
};
} // end namespace uh::dbn::storage::smart::sets

#endif //CORE_BLOCKED_MEMORY_H
