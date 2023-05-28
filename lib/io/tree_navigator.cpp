//
// Created by benjamin-elias on 28.05.23.
//

#include "tree_navigator.h"

#include "boost/algorithm/hex.hpp"

#include <filesystem>

namespace uh::io {

    tree_navigator::~tree_navigator() {
        std::for_each(sub_trees.begin(),sub_trees.end(),[](auto& sub_tree_entry){
            delete sub_tree_entry.first;
        });

        std::for_each(chunk_collections.begin(),chunk_collections.end(),[](auto& chunk_collection_entry){
            delete chunk_collection_entry.first;
        });
    }

    tree_navigator::tree_navigator(const std::filesystem::path& root): root(root){

        for(const auto&file_object:std::filesystem::directory_iterator(root))
        {
            if(file_object.path().filename() == "index")
                continue;

            uint8_t index_char = boost::algorithm::unhex(file_object.path().filename().string())[0];

            if(file_object.is_regular_file()){
                auto* tmp_cc = new chunk_collection(file_object.path());
                chunk_collections.emplace_back(tmp_cc,index_char);
            }

            if(file_object.is_directory()){
                auto* tmp_sub_tree = new tree_navigator(file_object.path());
                sub_trees.emplace_back(tmp_sub_tree,index_char);
            }
        }
    }

    serialization::fragment_serialize_size_format
    tree_navigator::write_indexed(std::span<const char> buffer, uint32_t alloc) {
        return serialization::fragment_serialize_size_format();
    }

    std::pair<std::vector<char>, serialization::fragment_serialize_size_format>
    tree_navigator::read_indexed(uint8_t at) {
        return std::pair<std::vector<char>, serialization::fragment_serialize_size_format>();
    }

    std::vector<serialization::fragment_serialize_size_format>
    tree_navigator::write_indexed_multi(const std::vector<std::span<const char>> &buffer) {
        return std::vector<serialization::fragment_serialize_size_format>();
    }

    std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format>>
    tree_navigator::read_indexed_multi(const std::vector<uint8_t> &at) {
        return std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format>>();
    }

    void tree_navigator::remove(uint8_t at) {

    }

    uint16_t tree_navigator::count() {
        return 0;
    }

    std::size_t tree_navigator::size() {
        return 0;
    }

    std::size_t tree_navigator::level_size() {
        return 0;
    }

    std::size_t tree_navigator::content_size() {
        return 0;
    }

    std::size_t tree_navigator::content_level_size() {
        return 0;
    }

    bool tree_navigator::full() const {
        return false;
    }

    uint64_t tree_navigator::free() {
        return 0;
    }

    std::filesystem::path tree_navigator::getRoot() {
        return std::filesystem::path();
    }


} // namespace uh::io