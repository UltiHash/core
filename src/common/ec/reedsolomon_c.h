#pragma once

#include "common/types/scoped_buffer.h"
#include "ec_interface.h"

#include <span>
#include <vector>

extern "C" {
#include <rs.h>
}

namespace uh::cluster {

static bool init_fec() {
    static bool init = false;
    if (!init) {
        fec_init();
        init = true;
    }
    return init;
};

class reedsolomon_c : public ec_interface {
public:
    reedsolomon_c(size_t data_nodes, size_t ec_nodes)
        : m_data_shards(data_nodes),
          m_parity_shards(ec_nodes),
          m_rs(get_rs()) {}

    void recover(const std::vector<std::span<const char>>& shards,
                 std::vector<data_stat>& stats) const override {
        if (shards.size() != m_parity_shards + m_data_shards and
            stats.size() != shards.size()) {
            throw std::logic_error(
                "Insufficient shards/stats to perform recovery");
        }

        const auto shard_size = shards.front().size();

        std::vector<const char*> ushards;
        ushards.reserve(shards.size());
        for (const auto& s : shards) {
            if (s.size() != shard_size) {
                throw std::logic_error(
                    "All shards must have the same size in the recovery");
            }
            ushards.emplace_back(s.data());
        }
        if (reed_solomon_reconstruct(
                m_rs.get(),
                reinterpret_cast<unsigned char**>(
                    const_cast<char**>(ushards.data())),
                reinterpret_cast<unsigned char*>(stats.data()),
                static_cast<int>(m_data_shards + m_parity_shards),
                static_cast<int>(shard_size)) != 0) {
            throw std::runtime_error("Could not recover the data");
        }
    }

    encoded encode(std::span<const char> data,
                   std::size_t shard_size = 0) const override {

        if (shard_size == 0) {
            shard_size = (data.size() + m_data_shards - 1) / m_data_shards;
        }

        const auto total_blocks = m_data_shards + m_parity_shards;

        std::vector<const char*> shard_ptrs;
        shard_ptrs.reserve(m_data_shards + m_parity_shards);

        size_t size = 0;
        // use existing allocation for shards as much as possible
        while (size + shard_size <= data.size()) {
            shard_ptrs.emplace_back(data.data() + size);
            size += shard_size;
        }

        // if the last shard is not filled completely, allocate a new
        // shard and copy the remaining data into it
        std::vector<unique_buffer<char>> new_shards;
        new_shards.reserve(m_data_shards - shard_ptrs.size() + m_parity_shards);

        if (const auto rem_size = data.size() - size; rem_size > 0) {
            new_shards.emplace_back(shard_size);
            std::memcpy(new_shards.back().data(), data.data() + size, rem_size);
            std::memset(new_shards.back().data() + rem_size, 0,
                        shard_size - rem_size);
            shard_ptrs.emplace_back(new_shards.back().data());
        }

        while (shard_ptrs.size() < m_data_shards) {
            new_shards.emplace_back(shard_size);
            std::memset(new_shards.back().data(), 0, shard_size);
            shard_ptrs.emplace_back(new_shards.back().data());
        }

        // create parity shards
        for (size_t i = 0; i < m_parity_shards; i++) {
            new_shards.emplace_back(shard_size);
            shard_ptrs.emplace_back(new_shards.back().data());
        }

        if (reed_solomon_encode2(m_rs.get(),
                                 reinterpret_cast<unsigned char**>(
                                     const_cast<char**>(shard_ptrs.data())),
                                 static_cast<int>(total_blocks),
                                 static_cast<int>(shard_size)) != 0) {
            throw std::runtime_error("Error in EC calculation");
        }

        encoded enc;
        enc.set(shard_ptrs, std::move(new_shards));
        return enc;
    }

private:
    [[nodiscard]] std::unique_ptr<reed_solomon, void (*)(reed_solomon*)>
    get_rs() const {
        if (m_parity_shards > 0) {
            return {reed_solomon_new(static_cast<int>(m_data_shards),
                                     static_cast<int>(m_parity_shards)),
                    [](reed_solomon* rs) { reed_solomon_release(rs); }};
        }
        return {nullptr, [](reed_solomon*) {}};
    }

    const size_t m_data_shards;
    const size_t m_parity_shards;
    bool m_init = init_fec();
    const std::unique_ptr<reed_solomon, void (*)(reed_solomon*)> m_rs;
};

} // end namespace uh::cluster
