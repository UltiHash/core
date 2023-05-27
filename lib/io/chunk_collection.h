//
// Created by benjamin-elias on 27.05.23.
//

#ifndef CORE_CHUNK_COLLECTION_H
#define CORE_CHUNK_COLLECTION_H

#include "serialization/fragment_size_struct.h"

#include <filesystem>
#include <vector>
#include <span>



namespace uh::io {

    class chunk_collection {

        /**
         * a chunk collection keeps track of the position and the movement
         * of incoming and outgoing chunks/fragments.
         * It only builds up a temporary connection to an underlying seekable_device.
         * On constructor the fragmented seekable device is scanned for the
         * position of its contents.
         *
         * @param collection_location where the file containing the chunk collection is located
         */
        explicit chunk_collection(const std::filesystem::path& collection_location);

        /**
         *
         * @return if the chunk collection is full
         */
        [[nodiscard]] bool full() const;

        /**
         * write with returning the index that was assigned to the written buffer
         * or instead give an index
         *
         * @param buffer to be written
         * @return tuple<stream size, assigned index position>
         */
        serialization::fragment_serialize_size_format write_indexed(std::span<const char> buffer);

        /**
         * read a certain index pointer and return a vector buffer of the content
         *
         * @param at index
         * @return buffer
         */
        std::pair<std::vector<char>,serialization::fragment_serialize_size_format> read_indexed(uint8_t at);

        /**
         * write indexed multiple buffers
         */
        std::vector<serialization::fragment_serialize_size_format>
        write_indexed(const std::vector<std::span<const char>>& buffer);

        /**
         * read indexed multiple positions with smart seeking
         */
        std::vector<std::vector<char>> read_indexed(const std::vector<uint8_t>& at);

        /**
         *
         * @return the count of addresses used
         */
        uint8_t count();

        /**
         *
         * @return the accumulated size of the chunk collection together with number of indexes used and further
         * header- and content size bytes
         */
        std::size_t size();

        /**
         *
         * @param index_adress is the address of a registered fragment/chunk
         * @return the size of the content payload of the fragment/chunk
         */
        std::size_t size(uint8_t index_adress);

        /**
         *
         * @param index_adress is the address of a registered fragment/chunk
         * @return the size of the content payload of the fragment/chunk
         */
        std::size_t size(std::size_t index_pos);

        /**
         *
         * @return path of chunk collection
         */
        std::filesystem::path getPath();

    private:
        std::filesystem::path path;
        std::vector<std::pair<serialization::fragment_serialize_size_format,std::streamoff>> index;
    };

} // namespace uh::io

#endif //CORE_CHUNK_COLLECTION_H
