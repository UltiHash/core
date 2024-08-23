#ifndef EC_CALCULATOR_H
#define EC_CALCULATOR_H

#include "common/types/scoped_buffer.h"

#include <list>
#include <string_view>
#include <vector>

namespace uh::cluster {

class ec_calculator {
public:
    struct encoded {
        [[nodiscard]] const std::vector<std::string_view>&
        get() const noexcept {
            return m_encoded;
        }

        void set(std::vector<std::string_view>&& enc) {
            m_encoded = std::move(enc);
        }

    private:
        friend ec_calculator;
        encoded() = default;
        std::list<unique_buffer<>> m_parities{};
        std::vector<std::string_view> m_encoded;
    };

    ec_calculator(size_t data_nodes, size_t ec_nodes)
        : m_data_nodes(data_nodes),
          m_ec_nodes(ec_nodes) {}

    [[nodiscard]] encoded encode(const std::string_view& data) const {
        encoded enc;
        enc.set({data});
        return enc;
    }

private:
    const size_t m_data_nodes;
    const size_t m_ec_nodes;
};

} // end namespace uh::cluster

#endif // EC_CALCULATOR_H
