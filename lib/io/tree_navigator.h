//
// Created by benjamin-elias on 28.05.23.
//

#ifndef CORE_TREE_NAVIGATOR_H
#define CORE_TREE_NAVIGATOR_H

#include "io/chunk_collection.h"

#include <vector>
#include <stack>
#include <mutex>
#include <array>

namespace uh::io
{

class tree_navigator
{

public:

    ~tree_navigator() = default;

    tree_navigator() = default;

    /**
     * a tree navigator takes care of 256 chunk collections and 256 tree navigators. The main job
     * is to balance storage location and size
     *
     * @param root is the root path of the tree
     */
    explicit tree_navigator(uint8_t set_name = 0,
                            tree_navigator* parent_navigator = nullptr,
                            const std::weak_ptr<std::filesystem::path>& root = std::make_shared<std::filesystem::path>());

    /**
     * write with returning the index that was assigned to the written buffer
     * or instead give an index
     *
     * @param buffer to be written
     * @param alloc allocate space and write fragment to chunk collection with the help of multiple buffers
     * WARNING: allocated space must be written completely (accumulated buffer size is longer or equal alloc)
     * @return tuple<stream size, assigned index position>
     */
    std::pair<std::stack<char>, serialization::fragment_serialize_size_format> write_indexed
        (std::span<const char> buffer,
         uint32_t alloc = 0,
         bool flush_after_operation = true,
         const std::stack<unsigned char>& maybe_force_stack_start = std::stack<unsigned char>{});

    /**
     * read a certain index pointer and return a vector buffer of the content
     *
     * @param at index
     * @return buffer
     */
    std::pair<std::vector<char>, serialization::fragment_serialize_size_format>
    read_indexed(const std::stack<char>& at, bool close_after_operation = false);

    /**
     * Write indexed multiple buffers and return a list of fragment size structs that contain written fragment
     * information.
     * Does not close filestream.
     */
    std::pair<std::stack<char>, std::vector<serialization::fragment_serialize_size_format>>
    write_indexed_multi(const std::vector<std::span<const char>>& buffer,
                        bool flush_after_operation = true);

    /**
     * read indexed multiple positions with smart seeking
     */
    std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format>>
    read_indexed_multi(const std::vector<std::stack<char>>& at, bool close_after_operation = false);

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
     * @return tell how many addresses are still free
     */
    uint64_t free();

    /**
     *
     * @return tell how many bytes more can be stored
     */
    uint64_t free_space();

    /**
     *
     * @return path of chunk collection
     */
    std::filesystem::path getRoot();

    /**
     *
     * @return name of tree_navigator
     */
    [[nodiscard]] const std::array<unsigned char, 2>& getTree_navigator_name() const;

private:
    std::weak_ptr<std::vector<std::pair<std::unique_ptr<tree_navigator>, uint8_t>>> sub_trees;
    std::weak_ptr<std::vector<std::pair<std::unique_ptr<chunk_collection>, uint8_t>>> chunk_collections;

    std::size_t size_stored{};
    std::weak_ptr<std::filesystem::path> tree_root;
    std::shared_ptr<tree_navigator> parent_navigator;
    std::array<unsigned char, 2> tree_navigator_name;

    std::recursive_mutex tree_work_mux{};
};

} // namespace uh::io

#endif //CORE_TREE_NAVIGATOR_H
