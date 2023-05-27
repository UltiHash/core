//
// Created by benjamin-elias on 27.05.23.
//

#include "chunk_collection.h"

namespace uh::io {


    chunk_collection::chunk_collection(const std::filesystem::path& collection_location) {

    }

    bool chunk_collection::full() const {

    }

    serialization::fragment_serialize_size_format chunk_collection::write_indexed(std::span<const char> buffer) {

    }

    std::pair<std::vector<char>, serialization::fragment_serialize_size_format>
    chunk_collection::read_indexed(uint8_t at) {

    }

    std::vector<serialization::fragment_serialize_size_format>
    chunk_collection::write_indexed(const std::vector<std::span<const char>> &buffer) {

    }

    std::vector<std::vector<char>>
    chunk_collection::read_indexed(const std::vector<uint8_t>& at){

    }

    uint8_t chunk_collection::count() {

    }

    std::size_t chunk_collection::size() {

    }

    std::size_t chunk_collection::size(uint8_t index_adress) {

    }

    std::filesystem::path chunk_collection::getPath() {
        return path;
    }
} // namespace uh::io