//
// Created by benjamin-elias on 28.05.23.
//

#include "tree_navigator.h"

#include "boost/algorithm/hex.hpp"

#include <util/exception.h>

#include <filesystem>
#include <stack>
#include <numeric>

namespace uh::io
{

// ---------------------------------------------------------------------

namespace
{

// ---------------------------------------------------------------------

std::array<unsigned char, 2> get_navigator_name(const std::filesystem::path& tree_root, uint8_t set_name)
{
    std::array<unsigned char, 2> out_name{};

    out_name[0] = boost::algorithm::unhex(tree_root.filename().string())[1];
    out_name[1] = set_name;

    return out_name;
}

// ---------------------------------------------------------------------

std::shared_ptr<std::filesystem::path> maybe_set_tree_root(const std::filesystem::path& tree_root, uint8_t set_name)
{
    std::filesystem::path out_path = tree_root;
    auto to_hex = boost::algorithm::hex(std::to_string(set_name));
    out_path /= to_hex;

    return std::make_shared<std::filesystem::path>(out_path);
}

// ---------------------------------------------------------------------

std::shared_ptr<std::vector<std::pair<std::shared_ptr<chunk_collection>,
                                      uint8_t>>> index_chunk_collections(const std::filesystem::path& input_path)
{
    std::shared_ptr<std::vector<std::pair<std::shared_ptr<chunk_collection>, uint8_t>>> out_chunk_collections{};

    for (const auto& file_object : std::filesystem::directory_iterator(input_path))
    {
        if (file_object.path().filename().string().size() == 1)
        THROW(util::exception,
              "Invalid object found in tree storage at location "
                  + (input_path / file_object.path().filename()).string() + " !");

        if (file_object.path().filename().string().size() > 2)
            continue;

        uint8_t index_char = boost::algorithm::unhex(file_object.path().filename().string())[0];

        if (file_object.is_regular_file())
        {
            out_chunk_collections->emplace_back(std::make_shared<chunk_collection>(file_object.path()), index_char);
        }
        else
        THROW(util::exception,
              "Invalid data file was a different object instead on chunk collection indexing, found in tree storage at location "
                  + (input_path / file_object.path().filename()).string() + " !");
    }

    return out_chunk_collections;
}

// ---------------------------------------------------------------------

std::shared_ptr<std::vector<std::pair<std::shared_ptr<tree_navigator>,
                                      uint8_t>>> index_sub_trees(const std::filesystem::path& input_path,
                                                                 std::array<unsigned char, 2> tree_name)
{
    std::shared_ptr<std::vector<std::pair<std::shared_ptr<tree_navigator>, uint8_t>>> out_sub_trees{};

    auto to_hex_string = [](unsigned char x)
    {
        return boost::algorithm::hex(std::to_string(x));
    };

    std::filesystem::path sub_tree_index_perisstence_path = input_path
        / (to_hex_string(tree_name[0]) + to_hex_string(tree_name[1]) + ".index");

    if(std::filesystem::exists(sub_tree_index_perisstence_path)){
        //parse sizes and create empty sub_trees
        std::size_t index_file_size_count{};
        std::size_t index_file_size = std::filesystem::file_size(sub_tree_index_perisstence_path);

        while (index_file_size_count < index_file_size){
            out_sub_trees->emplace_back();
        }
    }
    else{
        //index recursively and forget immediately on memory after persisting subtree index
        for (const auto& file_object : std::filesystem::directory_iterator(input_path))
        {
            if (file_object.path().filename().string().size() != 4 or file_object.path().extension() == ".index")
                continue;

            std::string index_char = boost::algorithm::unhex(file_object.path().filename().string());

            if (file_object.is_directory())
            {
                out_sub_trees
                    ->emplace_back(std::make_shared<tree_navigator>(index_char[1], input_path), index_char[1]);
            }
            else
            THROW(util::exception,
                  "Invalid data directory was a different object instead in sub tree indexing, found in tree storage at location "
                      + (input_path / file_object.path().filename()).string() + " !");
        }
    }

    return out_sub_trees;
}

// ---------------------------------------------------------------------

std::size_t accumulate_all_sizes(const std::weak_ptr<std::vector<std::pair<std::shared_ptr<tree_navigator>,
                                                                           uint8_t>>>& sub_trees,
                                 const std::weak_ptr<std::vector<std::pair<std::shared_ptr<chunk_collection>,
                                                                           uint8_t>>>& chunk_collections)
{
    auto locked_chunk_collections = chunk_collections.lock();

    std::size_t chunk_collection_size = std::accumulate(locked_chunk_collections->begin(),
                                                        locked_chunk_collections->end(),
                                                        (std::size_t) 0,
                                                        [](std::size_t acc,
                                                           const std::pair<std::shared_ptr<chunk_collection>,
                                                                           uint8_t>& pair_chunk_collection)
                                                        {
                                                            return acc + pair_chunk_collection.first->size();
                                                        });

    auto locked_sub_tree = sub_trees.lock();

    std::size_t sub_trees_size = std::accumulate(locked_sub_tree->begin(),
                                                 locked_sub_tree->end(),
                                                 (std::size_t) 0,
                                                 [](std::size_t acc,
                                                    const std::pair<std::shared_ptr<tree_navigator>,
                                                                    uint8_t>& pair_chunk_collection)
                                                 {
                                                     return acc + pair_chunk_collection.first->size();
                                                 });

    return chunk_collection_size + sub_trees_size;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

tree_navigator::tree_navigator(uint8_t set_name,
                               const std::filesystem::path& root,
                               std::size_t index_sub_trees_size)
    :
    tree_navigator_name(get_navigator_name(root, set_name)),
    tree_root(maybe_set_tree_root(root, set_name)),
    chunk_collections(index_chunk_collections(getRoot())),
    sub_trees(index_sub_trees(getRoot(), get_navigator_name(root, set_name))),
    size_stored(index_sub_trees_size + accumulate_all_sizes(sub_trees, chunk_collections))
{}

// ---------------------------------------------------------------------

std::pair<std::stack<char>, serialization::fragment_serialize_size_format<>>
tree_navigator::write_indexed(std::span<const char> buffer,
                              uint32_t alloc,
                              bool flush_after_operation,
                              const std::stack<unsigned char>& maybe_force_stack_start)
{
    // use index file
}

// ---------------------------------------------------------------------

std::pair<std::vector<char>, serialization::fragment_serialize_size_format<>>
tree_navigator::read_indexed(const std::stack<char>& at, bool close_after_operation)
{
    //TODO
}

// ---------------------------------------------------------------------

std::pair<std::stack<char>, std::vector<serialization::fragment_serialize_size_format<>>>
tree_navigator::write_indexed_multi(const std::vector<std::span<const char>>& buffer,
                                    bool flush_after_operation)
{

}

// ---------------------------------------------------------------------

std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format<>>>
tree_navigator::read_indexed_multi(const std::vector<std::stack<char>>& at, bool close_after_operation)
{

}

// ---------------------------------------------------------------------

void tree_navigator::remove(uint8_t at)
{

}

// ---------------------------------------------------------------------

uint16_t tree_navigator::count()
{

}

// ---------------------------------------------------------------------

std::size_t tree_navigator::size()
{

}

// ---------------------------------------------------------------------

std::size_t tree_navigator::level_size()
{

}

// ---------------------------------------------------------------------

std::size_t tree_navigator::content_size()
{

}

// ---------------------------------------------------------------------

std::size_t tree_navigator::content_level_size()
{

}

// ---------------------------------------------------------------------

uint64_t tree_navigator::free()
{

}

// ---------------------------------------------------------------------

uint64_t tree_navigator::free_space()
{

}

// ---------------------------------------------------------------------

std::filesystem::path tree_navigator::getRoot()
{
    //recursively ask parent for root. use tree name to accumulate
}

// ---------------------------------------------------------------------

const std::array<unsigned char, 2>& tree_navigator::getTree_navigator_name() const
{
    return tree_navigator_name;
}

// ---------------------------------------------------------------------

} // namespace uh::io