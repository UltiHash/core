//
// Created by benjamin-elias on 27.05.23.
//

#ifndef CORE_CHUNK_COLLECTION_H
#define CORE_CHUNK_COLLECTION_H

#include "serialization/fragment_size_struct.h"
#include "io/file.h"
#include "io/fragment_on_seekable_device.h"

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
        explicit chunk_collection(std::filesystem::path  collection_location);

        /**
         * write with returning the index that was assigned to the written buffer
         * or instead give an index
         *
         * @param buffer to be written
         * @param alloc allocate space and write fragment to chunk collection with the help of multiple buffers
         * WARNING: allocated space must be written completely (accumulated buffer size is longer or equal alloc)
         * @return tuple<stream size, assigned index position>
         */
        serialization::fragment_serialize_size_format write_indexed(std::span<const char> buffer,uint32_t alloc = 0);

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
        uint16_t count();

        /**
         *
         * @return the accumulated size of the chunk collection together with number of indexes used and further
         * header- and content size bytes
         */
        std::size_t size();

        /**
         *
         * @param index_adress is the address of a registered fragment/chunk
         * @return the size of the header and content payload of the fragment/chunk
         */
        std::size_t size(uint8_t index_adress);

        /**
         *
         * @param index_adress is the address of a registered fragment/chunk
         * @return the size of the content payload of the fragment/chunk
         */
        std::size_t content_size(uint8_t index_adress);

        /**
         *
         * @return if the chunk collection is full
         */
        [[nodiscard]] bool full() const;

        /**
         *
         * @return path of chunk collection
         */
        std::filesystem::path getPath();

    private:
        std::filesystem::path path;
        std::vector<std::pair<serialization::fragment_serialize_size_format,std::streamoff>> index;
        std::unique_ptr<io::file> temporarily_open_file;
        std::unique_ptr<io::fragmented_device> temporarily_cached_fragment_on_seekable_device;

        uint8_t next_free_address();
        std::vector<std::pair<serialization::fragment_serialize_size_format,std::streamoff>>::const_iterator
        find_address(uint8_t at);

        constexpr static std::size_t buf_size = 1 << 26;
    };

} // namespace uh::io

#endif //CORE_CHUNK_COLLECTION_H
