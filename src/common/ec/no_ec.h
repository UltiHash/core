#pragma once
#include "ec_interface.h"

namespace uh::cluster {

class no_ec : public ec_interface {
public:
    explicit no_ec(size_t data_nodes)
        : m_data_nodes(data_nodes) {}

    encoded encode(std::span<const char> data) const override {
        encoded enc;
        const auto shard_size = (data.size() + m_data_nodes - 1) / m_data_nodes;
        std::vector<std::span<const char>> shards;

        size_t size = 0;
        while (size < data.size()) {
            const auto adj_size = std::min(shard_size, data.size() - size);
            shards.emplace_back(data.subspan(size, adj_size));
            size += shard_size;
        }

        enc.set(std::move(shards));
        return enc;
    }

    void recover(const std::vector<std::span<const char>>& shards,
                 std::vector<data_stat>& stats) const override {
        for (const auto stat : stats) {
            if (stat == lost) {
                throw std::runtime_error("No ec nodes to perform the recovery");
            }
        }
    }

private:
    const size_t m_data_nodes;
};

} // end namespace uh::cluster
