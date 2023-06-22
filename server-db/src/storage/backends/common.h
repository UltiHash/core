//
// Created by masi on 6/21/23.
//

#ifndef CORE_COMMON_H
#define CORE_COMMON_H

#include <optional>
#include <functional>
#include <string_view>
#include <span>
#include <cstdint>
#include <memory>

#include "io/span_generator.h"

namespace uh::dbn::storage {

struct map_key_value {
    std::span <const char> key;
    std::span <const char> value;
    uint64_t offset {};

    map_key_value () = default;

    map_key_value (std::span <char> key_, std::span <char> value_):
        key (key_), value (value_) {}

    explicit map_key_value (const std::pair<uint64_t, std::string_view>& offset_data):
            offset (offset_data.first) {
        const auto key_size = *reinterpret_cast <const uint16_t*> (offset_data.second.data ());
        key = {offset_data.second.data() + sizeof (key_size), key_size};
        value = {offset_data.second.data() + sizeof (key_size) + key_size, offset_data.second.size() - sizeof (key_size) - key_size};
    }
};

struct index_type {
    std::uint64_t position {};
    int comp {};
};

struct set_result {
    std::optional <std::pair<uint64_t, std::string_view>> lower;
    std::optional <std::pair<uint64_t, std::string_view>> match;
    std::optional <std::pair<uint64_t, std::string_view>> upper;
    index_type index;
};

struct map_result {
    std::optional <map_key_value> lower;
    std::optional <map_key_value> match;
    std::optional <map_key_value> upper;
    index_type index;

    map_result () = default;

    map_result (std::span <char> key, std::span <char> value):
        match ({key, value}) {}

    explicit map_result (const set_result& set_res) {
        if (set_res.lower.has_value()) {
            lower = map_key_value {set_res.lower.value()};
        }
        if (set_res.match.has_value()) {
            match = map_key_value {set_res.match.value()};
        }
        if (set_res.upper.has_value()) {
            upper = map_key_value {set_res.upper.value()};
        }
        index = set_res.index;
    }

    explicit map_result (const std::pair<uint64_t, std::string_view>& offset_data): match (offset_data) {}

    private:


};

struct key_value_generator {
    std::unique_ptr <io::span_generator> key;
    std::unique_ptr <io::span_generator> value;
};

} // end namespace uh::dbn::storage
#endif //CORE_COMMON_H
