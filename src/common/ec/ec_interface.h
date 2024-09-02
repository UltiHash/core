#ifndef EC_INTERFACE_H
#define EC_INTERFACE_H
#include "common/types/scoped_buffer.h"

#include <string_view>
#include <vector>

namespace uh::cluster {

enum data_stat : uint8_t {
    valid = 0,
    lost = 1,
};

struct encoded {
    [[nodiscard]] const std::vector<std::string_view>& get() const noexcept {
        return m_encoded;
    }

    void set(const std::vector<const char*>& shard_ptrs,
             std::vector<unique_buffer<char>> new_shards) {
        const auto shard_size = new_shards.front().size();
        for (const char* ptr : shard_ptrs) {
            m_encoded.emplace_back(ptr, shard_size);
        }
        m_shard_allocations = std::move(new_shards);
    }

    void set(std::vector<std::string_view> shard_ptrs) {
        m_encoded = std::move(shard_ptrs);
    }

    std::vector<unique_buffer<char>> m_shard_allocations{};
    std::vector<std::string_view> m_encoded;
};

class ec_interface {
public:
    virtual void recover(const std::vector<std::string_view>& shards,
                         std::vector<data_stat>& stats) const = 0;

    [[nodiscard]] virtual encoded
    encode(const std::string_view& data) const = 0;
    virtual ~ec_interface() = default;
};

} // end namespace uh::cluster

#endif // EC_INTERFACE_H
