#pragma once

#include "common/types/scoped_buffer.h"

#include <span>
#include <vector>

namespace uh::cluster {

enum data_stat : uint8_t {
    valid = 0,
    lost = 1,
};

struct encoded {
    [[nodiscard]] const std::vector<std::span<const char>>& get() const {
        return m_encoded;
    }

    std::span<const char> get(std::size_t index) { return m_encoded.at(index); }

    void set(const std::vector<const char*>& shard_ptrs,
             std::vector<unique_buffer<char>> new_shards) {
        const auto shard_size = new_shards.front().size();
        for (const char* ptr : shard_ptrs) {
            m_encoded.emplace_back(ptr, shard_size);
        }
        m_shard_allocations = std::move(new_shards);
    }

    void set(std::vector<std::span<const char>> shard_ptrs) {
        m_encoded = std::move(shard_ptrs);
    }

    std::vector<unique_buffer<char>> m_shard_allocations{};
    std::vector<std::span<const char>> m_encoded;
};

} // end namespace uh::cluster
