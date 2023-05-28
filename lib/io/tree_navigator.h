//
// Created by benjamin-elias on 28.05.23.
//

#ifndef CORE_TREE_NAVIGATOR_H
#define CORE_TREE_NAVIGATOR_H

#include "io/chunk_collection.h"

#include <vector>

namespace uh::io {

    class tree_navigator {

    public:

        ~tree_navigator();

        /**
         * a tree navigator takes care of 256 chunk collections and 256 tree navigators. The main job
         * is to balance storage location and size
         *
         * @param root is the root path of the tree
         */
        explicit tree_navigator(const std::filesystem::path& root);

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
         * @param at remove fragment with this index
         */
        void remove(uint8_t at);

        /**
         *
         * @return the count of addresses used
         */
        uint16_t count();

        /**
         *
         * @return the accumulated size of the tree navigator with level size and all sub trees
         */
        std::size_t size();

        /**
         *
         * @return the size of all chunk collections of the tree navigator level
         */
        std::size_t level_size();

        /**
         *
         * @return the size of the content of the tree navigator from level size and all sub trees
         */
        std::size_t content_size();

        /**
         *
         * @return the size of the content of the tree navigator from level size
         */
        std::size_t content_level_size();

        /**
         *
         * @return if the tree navigator is full
         */
        [[nodiscard]] bool full() const;

        /**
         *
         * @return tell how many addresses are still free
         */
        uint64_t free();

        /**
         *
         * @return path of chunk collection
         */
        std::filesystem::path getRoot();

    private:
        std::vector<std::pair<tree_navigator*,uint8_t>> sub_trees;
        std::vector<std::pair<chunk_collection*,uint8_t>> chunk_collections;

        std::filesystem::path root;
    };

} // namespace uh::io

#endif //CORE_TREE_NAVIGATOR_H
