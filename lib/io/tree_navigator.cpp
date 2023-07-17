//
// Created by benjamin-elias on 28.05.23.
//

#include "tree_navigator.h"

#include "boost/algorithm/hex.hpp"

#include <util/exception.h>

#include <filesystem>
#include <stack>

namespace uh::io
{

// ---------------------------------------------------------------------

namespace
{

// ---------------------------------------------------------------------

std::array<unsigned char, 2> get_navigator_name(uint8_t set_name, const std::weak_ptr<tree_navigator>& parent_navigator)
{
    std::array<unsigned char, 2> out_name{};

    out_name[0] = parent_navigator.expired() ? 0 : parent_navigator.lock()->getTree_navigator_name()[1];
    out_name[1] = set_name;

    return out_name;
}

// ---------------------------------------------------------------------

std::shared_ptr<std::filesystem::path> maybe_set_tree_root(const std::weak_ptr<tree_navigator>& parent_navigator,
                                                           const std::weak_ptr<std::filesystem::path>& tree_root)
{
    if (parent_navigator.expired() and tree_root.lock()->empty())
    THROW(util::exception, "If parent navigator is expired, it is expected to bee super root, so requiring a "
                           "path for tree super root to be set!");

    if (tree_root.lock()->empty())
        return std::make_shared<std::filesystem::path>(parent_navigator.lock()->getRoot());

    return tree_root.lock();
}

// ---------------------------------------------------------------------

std::shared_ptr<std::vector<std::pair<std::unique_ptr<chunk_collection>,
                                      uint8_t>>> index_chunk_collections(const std::filesystem::path& input_path)
{
    std::vector<std::pair<std::unique_ptr<chunk_collection>, uint8_t>> out_chunk_collections;

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
            out_chunk_collections.emplace_back(std::make_unique<chunk_collection>(file_object.path()), index_char);
        }
        else
        THROW(util::exception,
              "Invalid data file was a different object instead, found in tree storage at location "
                  + (input_path / file_object.path().filename()).string() + " !");
    }
}

// ---------------------------------------------------------------------

std::shared_ptr<std::vector<std::pair<std::unique_ptr<tree_navigator>,
                                      uint8_t>>> index_sub_trees(const std::filesystem::path& input_path)
{
    if (file_object.is_directory())
    {
        auto* tmp_sub_tree = new tree_navigator(file_object.path(), <#initializer#>, <#initializer#>);
        sub_trees.emplace_back(tmp_sub_tree, index_char);
    }
}

// ---------------------------------------------------------------------

std::size_t accumulate_all_sizes(const std::weak_ptr<std::vector<std::pair<std::unique_ptr<tree_navigator>,
                                                                           uint8_t>>>& sub_trees,
                                 const std::weak_ptr<std::vector<std::pair<std::unique_ptr<chunk_collection>,
                                                                           uint8_t>>>& chunk_collections)
{

}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

tree_navigator::tree_navigator(uint8_t set_name,
                               const std::weak_ptr<tree_navigator>& parent_navigator,
                               const std::weak_ptr<std::filesystem::path>& root)
    :
    parent_navigator(parent_navigator),
    tree_navigator_name(get_navigator_name(set_name, parent_navigator)),
    tree_root(maybe_set_tree_root(parent_navigator, root)),
    chunk_collections(index_chunk_collections(getRoot())),
    sub_trees(index_sub_trees(getRoot())),
    size_stored(accumulate_all_sizes(sub_trees, chunk_collections))
{}

// ---------------------------------------------------------------------

std::pair<std::stack<char>, serialization::fragment_serialize_size_format>
tree_navigator::write_indexed(std::span<const char> buffer,
                              uint32_t alloc,
                              bool flush_after_operation,
                              const std::stack<unsigned char>& maybe_force_stack_start)
{
    std::vector<chunk_collection*> same_amount_addresses_free;

    uint16_t addresses_free_max{};
    for (const auto& col : chunk_collections)
    {
        uint16_t free_adr = col.first->free();
        if (free_adr > addresses_free_max)
            addresses_free_max = free_adr;
    }

    for (const auto& col : chunk_collections)
    {
        if (col.first->free() == addresses_free_max)
        {
            same_amount_addresses_free.push_back(col.first);
        }
    }

    //write to same amount_addresses_free where the alloc is most distant from the average fragment size

    auto min_size_of_most_free_addresses =
        *std::min_element(same_amount_addresses_free.begin(), same_amount_addresses_free.end(),
                          [](const auto& a, const auto& b)
                          {
                              return a->size() < b->size();
                          });

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