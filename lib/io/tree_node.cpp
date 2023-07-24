//
// Created by benjamin-elias on 28.05.23.
//

#include "tree_node.h"

#include "boost/algorithm/hex.hpp"

#include <util/exception.h>
#include <serialization/fragment_serialize_size_format.h>

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

std::shared_ptr<std::vector<std::pair<std::shared_ptr<tree_node>,
                                      uint8_t>>> index_sub_tree_persistent(const std::weak_ptr<std::vector<std::pair<std::shared_ptr<
    chunk_collection>, uint8_t>>>& chunk_collections,
                                                                           const std::filesystem::path& input_path,
                                                                           std::array<unsigned char, 2> tree_name,
                                                                           std::size_t& tree_navigator_size)
{
    std::shared_ptr<std::vector<std::pair<std::shared_ptr<tree_node>, uint8_t>>> out_sub_trees{};
    std::size_t chunk_collection_size{};
    std::size_t sub_trees_size{};

    auto locked_chunk_collections = chunk_collections.lock();

    chunk_collection_size = std::accumulate(locked_chunk_collections->begin(),
                                            locked_chunk_collections->end(),
                                            (std::size_t) 0,
                                            [](std::size_t acc,
                                               const std::pair<std::shared_ptr<chunk_collection>,
                                                               uint8_t>& pair_chunk_collection)
                                            {
                                                return acc + pair_chunk_collection.first->size();
                                            });

    auto to_hex_string = [](unsigned char x)
    {
        return boost::algorithm::hex(std::to_string(x));
    };

    std::filesystem::path sub_tree_index_perisstence_path = input_path
        / (to_hex_string(tree_name[0]) + to_hex_string(tree_name[1]) + ".index");

    if (std::filesystem::exists(sub_tree_index_perisstence_path))
    {
        //parse sizes and create empty sub_trees
        std::size_t index_file_size_count{};
        std::size_t index_file_size = std::filesystem::file_size(sub_tree_index_perisstence_path);

        io::file sub_tree_index_file(sub_tree_index_perisstence_path, std::ios_base::in);

        while (index_file_size_count < index_file_size)
        {
            serialization::fragment_serialize_size_format<uint64_t> tree_size_indexer{};
            tree_size_indexer.deserialize(sub_tree_index_file);
            sub_trees_size += tree_size_indexer.content_size;

            out_sub_trees->emplace_back(nullptr, tree_size_indexer.index_num);

            index_file_size_count += tree_size_indexer.serialized_size();
        }
    }
    else
    {
        io::file sub_tree_index_file(sub_tree_index_perisstence_path, std::ios_base::out);
        //index recursively and forget immediately on memory after persisting subtree index
        for (const auto& file_object : std::filesystem::directory_iterator(input_path))
        {
            if (file_object.path().filename().string().size() != 4 or file_object.path().extension() == ".index")
                continue;

            std::string index_char = boost::algorithm::unhex(file_object.path().filename().string());

            if (file_object.is_directory())
            {
                out_sub_trees
                    ->emplace_back(std::make_shared<tree_node>(index_char[1], input_path), index_char[1]);

                auto analyzed_size = out_sub_trees->back().first->size();
                sub_trees_size += analyzed_size;
                out_sub_trees->back().first.reset();

                serialization::fragment_serialize_size_format<uint64_t> write_to_sub_tree_index(index_char[1],analyzed_size);
                io::write(sub_tree_index_file, write_to_sub_tree_index.serialize());
            }
            else
            THROW(util::exception,
                  "Invalid data directory was a different object instead in sub tree indexing, found in tree storage at location "
                      + (input_path / file_object.path().filename()).string() + " !");
        }
    }

    tree_navigator_size = chunk_collection_size + sub_trees_size;

    return out_sub_trees;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

tree_node::tree_node(uint8_t set_name,
                     const std::filesystem::path& root)
    :
    tree_navigator_name(get_navigator_name(root, set_name)),
    tree_root(maybe_set_tree_root(root, set_name)),
    chunk_collections(index_chunk_collections(getRoot())),
    sub_trees(index_sub_tree_persistent(chunk_collections, getRoot(), get_navigator_name(root, set_name), size_stored))
{}

// ---------------------------------------------------------------------

std::pair<std::stack<char>, serialization::fragment_serialize_size_format<>>
tree_node::write_indexed(std::span<const char> buffer,
                         uint32_t alloc,
                         bool flush_after_operation,
                         const std::stack<unsigned char>& maybe_force_stack_start)
{
    // use index file
}

// ---------------------------------------------------------------------

std::pair<std::vector<char>, serialization::fragment_serialize_size_format<>>
tree_node::read_indexed(const std::stack<char>& at, bool close_after_operation)
{
    //TODO
}

// ---------------------------------------------------------------------

std::pair<std::stack<char>, std::vector<serialization::fragment_serialize_size_format<>>>
tree_node::write_indexed_multi(const std::vector<std::span<const char>>& buffer,
                               bool flush_after_operation)
{

}

// ---------------------------------------------------------------------

std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format<>>>
tree_node::read_indexed_multi(const std::vector<std::stack<char>>& at, bool close_after_operation)
{

}

// ---------------------------------------------------------------------

void tree_node::remove(uint8_t at)
{

}

// ---------------------------------------------------------------------

uint16_t tree_node::count()
{

}

// ---------------------------------------------------------------------

std::size_t tree_node::size()
{

}

// ---------------------------------------------------------------------

std::size_t tree_node::level_size()
{

}

// ---------------------------------------------------------------------

std::size_t tree_node::content_size()
{

}

// ---------------------------------------------------------------------

std::size_t tree_node::content_level_size()
{

}

// ---------------------------------------------------------------------

uint64_t tree_node::free()
{

}

// ---------------------------------------------------------------------

uint64_t tree_node::free_space()
{

}

// ---------------------------------------------------------------------

std::filesystem::path tree_node::getRoot()
{
    //recursively ask parent for root. use tree name to accumulate
}

// ---------------------------------------------------------------------

const std::array<unsigned char, 2>& tree_node::getTree_navigator_name() const
{
    return tree_navigator_name;
}

// ---------------------------------------------------------------------

} // namespace uh::io