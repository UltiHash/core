#pragma once

#include "common/types/scoped_buffer.h"
#include <random>
#include <string>

namespace uh::cluster {

// ---------------------------------------------------------------------

std::string random_string(
    std::size_t length = 16,
    const std::string& chars = "0123456789abcdefghijklmnopqrstuvwxyz");

// ---------------------------------------------------------------------

template <std::integral value_type> class random_generator {
public:
    random_generator(value_type min = {},
                     value_type max = std::numeric_limits<value_type>::max())
        : m_rg(std::random_device{}()),
          m_pick(min, max) {}

    value_type operator()() { return m_pick(m_rg); }

private:
    std::mt19937 m_rg;
    std::uniform_int_distribution<value_type> m_pick;
};

std::string generate_unique_id();

// ---------------------------------------------------------------------

} // namespace uh::cluster
