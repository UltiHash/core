//
// Created by benjamin-elias on 27.05.23.
//

#ifndef CORE_CHUNK_COLLECTION_H
#define CORE_CHUNK_COLLECTION_H

#include "io/file.h"
#include "io/fragment_on_seekable_device.h"
#include "serialization/fragment_size_struct.h"

#include <utility>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <span>

#define CHUNK_COLLECTION_BUFFER_SIZE 1 << 23


namespace uh::io {

    class chunk_collection {

    public:

        ~chunk_collection();

        /**
         * a chunk collection keeps track of the position and the movement
         * of incoming and outgoing chunks/fragments.
         * It only builds up a temporary connection to an underlying seekable_device.
         * On constructor the fragmented seekable device is scanned for the
         * position of its contents.
         *
         * @param collection_location where the file containing the chunk collection is located
         */
        explicit chunk_collection(std::filesystem::path collection_location,bool create_tempfile = false);

        /**
         * write with returning the index that was assigned to the written buffer
         * or instead give an index
         *
         * @param buffer to be written
         * @param alloc allocate space and write fragment to chunk collection with the help of multiple buffers
         * WARNING: allocated space must be written completely (accumulated buffer size is longer or equal alloc)
         * @return tuple<stream size, assigned index position>
         */
        serialization::fragment_serialize_size_format write_indexed
                (std::span<const char> buffer,uint32_t alloc = 0);

        /**
         * read a certain index pointer and return a vector buffer of the content
         *
         * @param at index
         * @return buffer
         */
        std::pair<std::vector<char>,serialization::fragment_serialize_size_format>
        read_indexed(uint8_t at);

        /**
         * Write indexed multiple buffers and return a list of fragment size structs that contain written fragment
         * information.
         * Does not close filestream.
         */
        std::vector<serialization::fragment_serialize_size_format>
        write_indexed_multi(const std::vector<std::span<const char>> &buffer);

        /**
         * read indexed multiple positions with smart seeking
         */
        std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format>>
        read_indexed_multi(const std::vector<uint8_t>& at);

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
         * @return tell how many addresses are still free
         */
        uint8_t free();

        /**
         *
         * @return path of chunk collection
         */
        std::filesystem::path getPath();

        /**
         * Rename the temp_file to `path` and make it a permanent file, in case we are based on temp_file
         *
         * @throw a file with the given name already exists
         */
        void release_to(const std::filesystem::path& release_path);

        std::vector<uint8_t> get_index_num_content_list() {
            std::vector<uint8_t> out_list(index.size());

            std::size_t counter{};

            for(const auto& item:index){
                out_list[counter] = item.first.index_num;
                counter++;
            }

            return out_list;
        }

    private:

        uint8_t next_free_address();

        std::vector<std::pair<serialization::fragment_serialize_size_format,std::streamoff>>::const_iterator
        find_address(uint8_t at);

        bool to_be_deleted;
        std::filesystem::path path;
        std::vector<std::pair<serialization::fragment_serialize_size_format,std::streamoff>> index;
    };

} // namespace uh::io

#endif //CORE_CHUNK_COLLECTION_H
