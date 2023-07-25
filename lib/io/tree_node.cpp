//
// Created by benjamin-elias on 28.05.23.
//

#include "tree_node.h"

#include "boost/algorithm/hex.hpp"

#include <util/exception.h>
#include <serialization/tree_node_serialize_size_format.h>

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
                                      uh::serialization::tree_node_serialize_size_format>>>
index_chunk_collections(const std::filesystem::path& input_path,
                        std::size_t& tree_node_size,
                        std::size_t& tree_node_chunk_count)
{
    std::shared_ptr<std::vector<std::pair<std::shared_ptr<chunk_collection>,
                                          uh::serialization::tree_node_serialize_size_format>>> out_chunk_collections{};

    std::string root_index_name = input_path.filename().replace_extension(".index").string();

    if (not std::filesystem::exists(std::filesystem::path(input_path) / root_index_name))
        for (const auto& file_object: std::filesystem::directory_iterator(input_path))
        {
            if (file_object.path().filename().string().size() == 1)
            THROW(util::exception,
                  "Invalid object found in tree storage at location "
                      + (input_path / file_object.path().filename()).string() + " !");

            if (file_object.path().filename().string().size() > 2)
                continue;

            if (file_object.is_regular_file())
            {
                auto cc = std::make_shared<chunk_collection>(file_object.path());
                auto index_char =
                    uh::serialization::tree_node_serialize_size_format(boost::algorithm::unhex(file_object.path()
                                                                                                   .filename()
                                                                                                   .string())[0],
                                                                       cc->size(),
                                                                       uh::serialization::CHUNK_COLLECTION,
                                                                       cc->count());

                out_chunk_collections->emplace_back(cc, index_char);
                tree_node_size += cc->size();
                tree_node_chunk_count += cc->count();
                out_chunk_collections->back().first.reset();
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
                                      uh::serialization::tree_node_serialize_size_format>>>
index_sub_tree_persistent(const std::weak_ptr<std::vector<std::pair<std::shared_ptr<
    chunk_collection>, uh::serialization::tree_node_serialize_size_format>>>& chunk_collections,
                          const std::filesystem::path& input_path,
                          std::size_t& tree_node_size,
                          std::size_t& tree_node_chunk_count)
{
    std::shared_ptr<std::vector<std::pair<std::shared_ptr<tree_node>,
                                          uh::serialization::tree_node_serialize_size_format>>> out_sub_trees{};

    auto locked_chunk_collections = chunk_collections.lock();

    std::string root_index_name = input_path.filename().replace_extension(".index").string();
    std::filesystem::path sub_tree_index_perisstence_path = std::filesystem::path(input_path) / root_index_name;

    if (std::filesystem::exists(sub_tree_index_perisstence_path))
    {
        std::size_t index_file_size_count{};
        std::size_t index_file_size = std::filesystem::file_size(sub_tree_index_perisstence_path);

        io::file sub_tree_index_file(sub_tree_index_perisstence_path, std::ios_base::in);

        while (index_file_size_count < index_file_size)
        {
            serialization::tree_node_serialize_size_format tree_size_indexer{};
            tree_size_indexer.deserialize(sub_tree_index_file);

            switch (tree_size_indexer.tree_type)
            {
                case uh::serialization::TREE_NODE:
                    out_sub_trees->emplace_back(nullptr, tree_size_indexer);
                    break;
                case uh::serialization::CHUNK_COLLECTION:
                    locked_chunk_collections->emplace_back(nullptr, tree_size_indexer);
                    break;
                default:
                THROW(util::exception, "tree_node type not supported!");
            }

            tree_node_size += tree_size_indexer.content_size;
            tree_node_chunk_count += tree_size_indexer.chunk_num;

            index_file_size_count += tree_size_indexer.serialized_size();
        }
    }
    else
    {
        io::file sub_tree_index_file(sub_tree_index_perisstence_path, std::ios_base::out);
        //index recursively and forget immediately on memory after persisting subtree index
        for (const auto& file_object: std::filesystem::directory_iterator(input_path))
        {
            if (file_object.path().filename().string().size() != 4 or file_object.path().extension() == ".index")
                continue;

            if (file_object.is_directory())
            {
                unsigned char index_char = boost::algorithm::unhex(file_object.path().filename().string())[1];

                auto tn = std::make_shared<tree_node>(input_path, index_char);
                serialization::tree_node_serialize_size_format tree_size_indexer(index_char,
                                                                                 tn->size(),
                                                                                 uh::serialization::TREE_NODE,
                                                                                 tn->count());

                out_sub_trees->emplace_back(tn, tree_size_indexer);
                tree_node_size += tn->size();
                tree_node_chunk_count += tn->count();
                out_sub_trees->back().first.reset();

                serialization::tree_node_serialize_size_format
                    write_to_sub_tree_index(index_char,
                                            tn->size(),
                                            uh::serialization::TREE_NODE,
                                            tn->count());

                io::write(sub_tree_index_file, write_to_sub_tree_index.serialize());
            }
            else
            THROW(util::exception,
                  "Invalid data directory was a different object instead in sub tree indexing, found in tree storage at location "
                      + (input_path / file_object.path().filename()).string() + " !");
        }

        for(auto& cc: *locked_chunk_collections){
            io::write(sub_tree_index_file, cc.second.serialize());
        }
    }

    return out_sub_trees;
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

tree_node::tree_node(const std::filesystem::path& root, uint8_t set_name)
    :
    tree_navigator_name(get_navigator_name(root, set_name)),
    tree_root(maybe_set_tree_root(root, set_name)),
    chunk_collections(index_chunk_collections(getRoot(), size_stored, chunk_count)),
    sub_trees(index_sub_tree_persistent(chunk_collections, getRoot(), size_stored, chunk_count))
{}

// ---------------------------------------------------------------------

std::pair<std::stack<char>, serialization::fragment_serialize_size_format<>>
tree_node::write_indexed(std::span<const char> buffer,
                         uint32_t alloc,
                         bool flush_after_operation,
                         const std::stack<unsigned char>& maybe_force_stack_start)
{
    // use index file
    std::lock_guard lock(tree_work_mux);
}

// ---------------------------------------------------------------------

std::pair<std::vector<char>, serialization::fragment_serialize_size_format<>>
tree_node::read_indexed(const std::stack<char>& at, bool close_after_operation)
{
    //TODO
    std::lock_guard lock(tree_work_mux);
}

// ---------------------------------------------------------------------

std::pair<std::stack<char>, std::vector<serialization::fragment_serialize_size_format<>>>
tree_node::write_indexed_multi(const std::vector<std::span<const char>>& buffer,
                               bool flush_after_operation)
{
    std::lock_guard lock(tree_work_mux);
}

// ---------------------------------------------------------------------

std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format<>>>
tree_node::read_indexed_multi(const std::vector<std::stack<char>>& at, bool close_after_operation)
{
    std::lock_guard lock(tree_work_mux);
}

// ---------------------------------------------------------------------

void tree_node::remove(uint8_t at)
{
    std::lock_guard lock(tree_work_mux);
}

// ---------------------------------------------------------------------

uint64_t tree_node::count()
{
    std::lock_guard lock(tree_work_mux);
}

// ---------------------------------------------------------------------

std::size_t tree_node::size()
{
    std::lock_guard lock(tree_work_mux);
}

// ---------------------------------------------------------------------

std::size_t tree_node::level_size()
{
    std::lock_guard lock(tree_work_mux);
}

// ---------------------------------------------------------------------

std::size_t tree_node::content_size()
{
    std::lock_guard lock(tree_work_mux);
}

// ---------------------------------------------------------------------

std::size_t tree_node::content_level_size()
{
    std::lock_guard lock(tree_work_mux);
}

// ---------------------------------------------------------------------

uint64_t tree_node::free()
{
    std::lock_guard lock(tree_work_mux);
}

// ---------------------------------------------------------------------

uint64_t tree_node::free_space()
{
    std::lock_guard lock(tree_work_mux);
}

// ---------------------------------------------------------------------

std::filesystem::path tree_node::getRoot()
{
    std::lock_guard lock(tree_work_mux);
    return *tree_root.lock();
}

// ---------------------------------------------------------------------

const std::array<unsigned char, 2>& tree_node::getTree_navigator_name() const
{
    return tree_navigator_name;
}

// ---------------------------------------------------------------------

} // namespace uh::io