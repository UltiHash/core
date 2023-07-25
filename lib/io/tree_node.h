//
// Created by benjamin-elias on 28.05.23.
//

#ifndef CORE_TREE_NAVIGATOR_H
#define CORE_TREE_NAVIGATOR_H

#include <io/chunk_collection.h>
#include <serialization/tree_node_serialize_size_format.h>

#include <vector>
#include <stack>
#include <mutex>
#include <array>

namespace uh::io
{

class tree_node
{

public:

    ~tree_node();

    tree_node() = default;

    /**
     * a tree navigator takes care of 256 chunk collections and 256 tree navigators. The main job
     * is to balance storage location and size
     *
     * @param root is the root path of the tree
     */
    explicit tree_node(const std::filesystem::path& root, uint8_t set_name = 0);

    /**
     * write with returning the index that was assigned to the written navigator
     * or instead give an index
     *
     * @param buffer to be written
     * @param alloc allocate space and write fragment to chunk collection with the help of multiple buffers
     * WARNING: allocated space must be written completely (accumulated navigator size is longer or equal alloc)
     * @return tuple<stream size, assigned index position>
     */
    std::pair<std::stack<unsigned char>, uh::serialization::tree_node_serialize_size_format> write_indexed
        (std::vector<char> buffer,
         uint32_t alloc,
         bool flush_after_operation,
         std::stack<unsigned char> maybe_force_stack_start = std::stack<unsigned char>{},
         std::stack<unsigned char> navigator = std::stack<unsigned char>{});

    /**
     * read a certain index pointer and return a vector buffer of the content
     *
     * @param at index
     * @return buffer
     */
    std::pair<std::vector<char>, serialization::fragment_serialize_size_format<>>
    read_indexed(std::stack<unsigned char> at, bool close_after_operation = false);

    /**
     * Write indexed multiple buffers and return a list of fragment size structs that contain written fragment
     * information.
     * Does not close filestream.
     */
    std::vector<std::pair<std::stack<char>, std::vector<serialization::fragment_serialize_size_format<>>>>
    write_indexed_multi(const std::vector<std::span<unsigned char>> buffer,
                        bool flush_after_operation = true);

    /**
     * read indexed multiple positions with smart seeking
     */
    std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format<>>>
    read_indexed_multi(const std::vector<std::stack<unsigned char>> at, bool close_after_operation = false);

    /**
     *
     * @param at remove fragment with this index
     */
    void remove(std::stack<unsigned char> at);

    /**
     *
     * @return the count of chunks within this tree_node
     */
    uint64_t count();

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
     * @return the size of the content of the tree navigator from level size (only chunk collections)
     */
    std::size_t content_level_size();

    /**
     *
     * @return tell how many addresses are not completely filled up on all chunk collections recursively
     */
    uint64_t free_count();

    /**
     *
     * @return path of chunk collection
     */
    std::filesystem::path getRoot();

    /**
     *
     * @return name of tree_node
     */
    [[nodiscard]] const std::array<unsigned char, 2>& getTree_navigator_name() const;

    /**
     *
     * @return if all addresses of chunk collections are used up on the tree node
     */
    bool chunk_collections_address_full();

    /**
     *
     * @return if all addresses of sub tree nodes are used up on the tree node
     */
    bool sub_trees_address_full();

private:
    std::weak_ptr<std::vector<std::pair<std::shared_ptr<tree_node>, uh::serialization::tree_node_serialize_size_format>>> sub_trees;
    std::weak_ptr<std::vector<std::pair<std::shared_ptr<chunk_collection>, uh::serialization::tree_node_serialize_size_format>>> chunk_collections;

    std::size_t size_stored{};
    std::size_t chunk_count{};
    std::weak_ptr<std::filesystem::path> tree_root;
    std::array<unsigned char, 2> tree_navigator_name{};

    std::recursive_mutex tree_work_mux{};

    auto min_address_min_size_filter(auto search_pair_vector){
        decltype(search_pair_vector)
            sub_trees_min_address;

        uint64_t min_addr = std::numeric_limits<uint64_t>::max();

        for(const auto& el: search_pair_vector){
            if(el.second.chunk_num < min_addr){
                sub_trees_min_address.clear();
                min_addr = el.second.chunk_num;
            }
            if(el.second.chunk_num == min_addr)
                sub_trees_min_address.emplace_back(el);
        }

        decltype(search_pair_vector)
            sub_trees_min_address_min_size;

        uint64_t min_size = std::numeric_limits<uint64_t>::max();

        for(const auto& el: sub_trees_min_address){
            if(el.second.content_size < min_size){
                sub_trees_min_address_min_size.clear();
                min_size = el.second.chunk_num;
            }
            if(el.second.content_size == min_size)
                sub_trees_min_address_min_size.emplace_back(el);
        }

        std::sort(sub_trees_min_address_min_size.begin(),
                  sub_trees_min_address_min_size.end(),
                  [](const auto &item1, const auto &item2)
                  {
                      return item1.second.index_num < item2.second.index_num;
                  });

        return sub_trees_min_address_min_size;
    }

protected:
    std::pair<std::shared_ptr<chunk_collection>, uh::serialization::tree_node_serialize_size_format> get_chunk_collection_indexed(uint8_t at);
    std::pair<std::shared_ptr<tree_node>, uh::serialization::tree_node_serialize_size_format> get_tree_node_indexed(uint8_t at);

    uint8_t next_free_tree_node_address();
    uint8_t next_free_chunk_collection_address();
};

} // namespace uh::io

#endif //CORE_TREE_NAVIGATOR_H
