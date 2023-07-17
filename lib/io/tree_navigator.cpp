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

std::array<unsigned char, 2> get_navigator_name(uint8_t set_name, tree_navigator* parent_navigator)
{
    std::array<unsigned char, 2> out_name{};

    out_name[0] = parent_navigator == nullptr ? 0 : parent_navigator->getTree_navigator_name()[1];
    out_name[1] = set_name;

    return out_name;
}

// ---------------------------------------------------------------------

std::shared_ptr<std::filesystem::path> maybe_set_tree_root(tree_navigator* parent_navigator,
                                                           const std::weak_ptr<std::filesystem::path>& tree_root)
{
    if (tree_root.lock()->empty())
    THROW(util::exception, "If parent navigator is expired, it is expected to bee super root, so requiring a "
                           "path for tree super root to be set!");

    if (tree_root.lock()->empty())
        return std::make_shared<std::filesystem::path>(parent_navigator->getRoot());

    return tree_root.lock();
}

// ---------------------------------------------------------------------

std::shared_ptr<std::vector<std::pair<std::unique_ptr<chunk_collection>,
                                      uint8_t>>> index_chunk_collections(const std::filesystem::path& input_path)
{
    std::shared_ptr<std::vector<std::pair<std::unique_ptr<chunk_collection>, uint8_t>>> out_chunk_collections{};

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
            out_chunk_collections->emplace_back(std::make_unique<chunk_collection>(file_object.path()), index_char);
        }
        else
        THROW(util::exception,
              "Invalid data file was a different object instead on chunk collection indexing, found in tree storage at location "
                  + (input_path / file_object.path().filename()).string() + " !");
    }

    return out_chunk_collections;
}

// ---------------------------------------------------------------------

std::shared_ptr<std::vector<std::pair<std::unique_ptr<tree_navigator>,
                                      uint8_t>>> index_sub_trees(const std::filesystem::path& input_path, tree_navigator* parent_navigator)
{
    std::shared_ptr<std::vector<std::pair<std::unique_ptr<tree_navigator>, uint8_t>>> out_sub_trees{};

    for (const auto& file_object : std::filesystem::directory_iterator(input_path))
    {
        if (file_object.path().filename().string().size() != 4)
            continue;

        std::string index_char = boost::algorithm::unhex(file_object.path().filename().string());

        if (file_object.is_directory())
        {
            out_sub_trees->emplace_back(std::make_unique<tree_navigator>(index_char[1], parent_navigator), index_char[1]);
        }
        else
        THROW(util::exception,
              "Invalid data directory was a different object instead in sub tree indexing, found in tree storage at location "
                  + (input_path / file_object.path().filename()).string() + " !");
    }

    return out_sub_trees;
}

// ---------------------------------------------------------------------

std::size_t accumulate_all_sizes(const std::weak_ptr<std::vector<std::pair<std::unique_ptr<tree_navigator>,
                                                                           uint8_t>>>& sub_trees,
                                 const std::weak_ptr<std::vector<std::pair<std::unique_ptr<chunk_collection>,
                                                                           uint8_t>>>& chunk_collections)
{
    std::size_t chunk_collection_size = std::accumulate();
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

tree_navigator::tree_navigator(uint8_t set_name,
                               tree_navigator* parent_navigator,
                               const std::weak_ptr<std::filesystem::path>& root)
    :
    parent_navigator(parent_navigator),
    tree_navigator_name(get_navigator_name(set_name, parent_navigator)),
    tree_root(maybe_set_tree_root(parent_navigator, root)),
    chunk_collections(index_chunk_collections(getRoot())),
    sub_trees(index_sub_trees(getRoot(), this)),
    size_stored(accumulate_all_sizes(sub_trees, chunk_collections))
{}

// ---------------------------------------------------------------------

std::pair<std::stack<char>, serialization::fragment_serialize_size_format>
tree_navigator::write_indexed(std::span<const char> buffer,
                              uint32_t alloc,
                              bool flush_after_operation,
                              const std::stack<unsigned char>& maybe_force_stack_start)
{
    // use index file
}

// ---------------------------------------------------------------------

std::pair<std::vector<char>, serialization::fragment_serialize_size_format>
tree_navigator::read_indexed(const std::stack<char>& at, bool close_after_operation)
{
    //TODO
}

// ---------------------------------------------------------------------

std::pair<std::stack<char>, std::vector<serialization::fragment_serialize_size_format>>
tree_navigator::write_indexed_multi(const std::vector<std::span<const char>>& buffer,
                                    bool flush_after_operation)
{

}

// ---------------------------------------------------------------------

std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format>>
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