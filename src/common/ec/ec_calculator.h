#ifndef EC_CALCULATOR_H
#define EC_CALCULATOR_H

#include "common/types/scoped_buffer.h"

#include <list>
#include <string_view>
#include <vector>

extern "C" {
#include <rs.h>
}

namespace uh::cluster {

class ec_calculator {
public:
    enum data_stat : uint8_t {
        valid = 0,
        lost = 1,
    };

    struct encoded {

        [[nodiscard]] const std::vector<std::string_view>&
        get() const noexcept {
            return m_encoded;
        }

        void set(const std::vector<unsigned char*>& shard_ptrs,
                 std::vector<unique_buffer<unsigned char>> new_shards) {
            const auto shard_size = new_shards.front().size();
            for (unsigned char* ptr : shard_ptrs) {
                m_encoded.emplace_back(reinterpret_cast<char*>(ptr),
                                       shard_size);
            }
            m_parities = std::move(new_shards);
        }

        void set(std::vector<std::string_view> shard_ptrs) {
            m_encoded = std::move(shard_ptrs);
        }

    private:
        friend ec_calculator;
        encoded() = default;
        std::vector<unique_buffer<unsigned char>> m_parities{};
        std::vector<std::string_view> m_encoded;
    };

    ec_calculator(size_t data_nodes, size_t ec_nodes)
        : m_data_nodes(data_nodes),
          m_ec_nodes(ec_nodes),
          m_rs(reed_solomon_new(m_data_nodes, m_ec_nodes)) {}

    void recover(const std::vector<std::span<char>>& shards,
                 std::vector<data_stat>& stats) const {
        if (shards.size() != m_ec_nodes + m_data_nodes and
            stats.size() != shards.size()) {
            throw std::logic_error(
                "Insufficient shards/stats to perform recovery");
        }

        const auto shard_size = shards.front().size();
        if (shard_size % BLOCK_SIZE != 0) {
            throw std::logic_error("Invalid shard size for recovery");
        }

        std::vector<unsigned char*> ushards;
        ushards.reserve(shards.size());
        for (const auto& s : shards) {
            if (s.size() != shard_size) {
                throw std::logic_error(
                    "All shards must have the same size in the recovery");
            }
            ushards.emplace_back(reinterpret_cast<unsigned char*>(s.data()));
        }
        const auto block_count = shard_size / BLOCK_SIZE * shards.size();
        if (reed_solomon_reconstruct(
                m_rs, ushards.data(),
                reinterpret_cast<unsigned char*>(stats.data()),
                static_cast<int>(block_count), BLOCK_SIZE) != 0) {
            throw std::runtime_error("Could not recover the data");
        }
    }

    [[nodiscard]] encoded encode(const std::string_view& data) const {

        encoded enc;

        if (m_ec_nodes == 0) {

            const auto shard_size =
                (data.size() + m_data_nodes - 1) / m_data_nodes;
            std::vector<std::string_view> shards;

            size_t size = 0;
            while (size < data.size()) {
                const auto adj_size = std::min(shard_size, data.size() - size);
                shards.emplace_back(data.substr(size, adj_size));
                size += shard_size;
            }

            enc.set(shards);
        } else {
            auto block_count =
                data.size() + (data.size() - data.size() % BLOCK_SIZE);
            block_count =
                block_count + (block_count - block_count % m_data_nodes);
            const auto shard_block_count = block_count / m_data_nodes;
            const auto total_blocks =
                shard_block_count * (m_data_nodes + m_ec_nodes);
            const auto shard_size = shard_block_count * BLOCK_SIZE;

            std::vector<unsigned char*> shard_ptrs;
            shard_ptrs.reserve(m_data_nodes + m_ec_nodes);

            size_t size = 0;
            // use existing allocation for shards as much as possible
            while (size + shard_size <= data.size()) {
                shard_ptrs.emplace_back((unsigned char*)(data.data()) + size);
                size += shard_size;
            }

            // if the last shard is not filled completely, allocate a new
            // shard and copy the remaining data into it

            std::vector<unique_buffer<unsigned char>> new_shards;
            new_shards.reserve(m_ec_nodes + 1);

            if (const auto rem_size = data.size() - size; rem_size > 0) {
                new_shards.emplace_back(shard_size);
                std::memcpy(new_shards.back().data(), data.data() + size,
                            rem_size);
                std::memset(new_shards.back().data() + rem_size, 0,
                            shard_size - rem_size);
                shard_ptrs.emplace_back(new_shards.back().data());
            }

            // create parity shards
            for (size_t i = 0; i < m_ec_nodes; i++) {
                new_shards.emplace_back(shard_size);
                shard_ptrs.emplace_back(new_shards.back().data());
            }

            if (reed_solomon_encode2(m_rs, shard_ptrs.data(),
                                     static_cast<int>(total_blocks),
                                     BLOCK_SIZE) != 0) {
                throw std::runtime_error("Error in EC calculation");
            }

            enc.set(shard_ptrs, std::move(new_shards));
        }

        return enc;
    }

    ~ec_calculator() { reed_solomon_release(m_rs); }

private:
    const size_t m_data_nodes;
    const size_t m_ec_nodes;
    constexpr static int BLOCK_SIZE = 8 * KIBI_BYTE;
    reed_solomon* m_rs;
};

} // end namespace uh::cluster

#endif // EC_CALCULATOR_H
