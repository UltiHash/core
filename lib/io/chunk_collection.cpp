//
// Created by benjamin-elias on 27.05.23.
//

#include "chunk_collection.h"

#include "io/file.h"
#include "io/fragment_on_seekable_device.h"
#include "serialization/fragment_size_struct.h"

#include <utility>
#include <filesystem>

namespace uh::io {


    chunk_collection::chunk_collection(std::filesystem::path collection_location):
    path(std::move(collection_location))
    {
        if(std::filesystem::exists(path)){

        }
        file f1(path,std::ios_base::in);
        fragment_on_seekable_device frag(f1);

        std::streamoff collection_offset{};
        uint16_t index_entry_count{};
        serialization::fragment_serialize_size_format skip_format(0,0,0);

        do{
            skip_format = frag.skip();

            if(skip_format.content_size == 0)break;

            if(index_entry_count >= std::numeric_limits<unsigned char>::max())
                THROW(util::exception,"Indexing of chunk collection "+path.string()+" exceeded limits");

            index.emplace_back(skip_format,collection_offset);
            index_entry_count++;

        } while (skip_format.content_size > 0);
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

    bool chunk_collection::full() const {
        return index.size() == std::numeric_limits<unsigned char>::max();
    }

    std::filesystem::path chunk_collection::getPath() {
        return path;
    }
} // namespace uh::io