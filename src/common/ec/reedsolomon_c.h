#pragma once

#include <span>
#include <vector>

extern "C" {
#include <rs.h>
}

namespace uh::cluster {

enum data_stat : uint8_t {
    valid = 0,
    lost = 1,
};

static bool init_fec() {
    static bool init = false;
    if (!init) {
        fec_init();
        init = true;
    }
    return init;
};

class reedsolomon_c {
public:
    reedsolomon_c(std::size_t data_nodes, std::size_t ec_nodes,
                  std::size_t shard_size = 0)
        : m_data_shards(data_nodes),
          m_parity_shards(ec_nodes),
          m_shard_size(shard_size),
          m_rs(get_rs()) {}

    void recover(std::vector<std::span<char>>& shards,
                 std::vector<data_stat>& stats) const {
        if (shards.size() != m_parity_shards + m_data_shards and
            stats.size() != shards.size()) {
            throw std::logic_error(
                "Insufficient shards/stats to perform recovery");
        }

        const auto shard_size = shards.front().size();
        if (m_shard_size != 0 and m_shard_size != shard_size) {
            throw std::logic_error(
                "Shard size mismatch between shards and configuration");
        }

        std::vector<char*> ushards;
        ushards.reserve(shards.size());
        for (const auto& s : shards) {
            if (s.size() != shard_size) {
                throw std::logic_error(
                    "All shards must have the same size in the recovery");
            }
            ushards.emplace_back(s.data());
        }
        if (reed_solomon_reconstruct(
                m_rs.get(), reinterpret_cast<unsigned char**>(ushards.data()),
                reinterpret_cast<unsigned char*>(stats.data()),
                static_cast<int>(m_data_shards + m_parity_shards),
                static_cast<int>(shard_size)) != 0) {
            throw std::runtime_error("Could not recover the data");
        }
    }

    void encode(std::vector<std::span<const char>>& data,
                std::vector<std::span<char>>& parity) const {
        // NOTE: we don't need to initialize parities

        auto begins = data | std::views::transform([](const auto& s) {
                          return reinterpret_cast<unsigned char*>(
                              const_cast<char*>(s.data()));
                      });
        auto p_shards =
            std::vector<unsigned char*>(begins.begin(), begins.end());

        for (const auto& p : parity) {
            p_shards.push_back(reinterpret_cast<unsigned char*>(p.data()));
        }

        if (reed_solomon_encode2(m_rs.get(), p_shards.data(),
                                 m_data_shards + m_parity_shards,
                                 m_shard_size) != 0) {
            throw std::runtime_error("Error in EC calculation");
        }
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

    const std::size_t m_data_shards;
    const std::size_t m_parity_shards;
    const std::size_t m_shard_size;
    bool m_init = init_fec();
    const std::unique_ptr<reed_solomon, void (*)(reed_solomon*)> m_rs;
};

} // end namespace uh::cluster
