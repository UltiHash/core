//
// Created by benjamin-elias on 28.05.23.
//

#include "tree_navigator.h"

#include "boost/algorithm/hex.hpp"

#include <filesystem>
#include <stack>

namespace uh::io {

    // ---------------------------------------------------------------------

    tree_navigator::~tree_navigator() {
        std::for_each(sub_trees.begin(),sub_trees.end(),[](auto& sub_tree_entry){
            delete sub_tree_entry.first;
        });

        std::for_each(chunk_collections.begin(),chunk_collections.end(),[](auto& chunk_collection_entry){
            delete chunk_collection_entry.first;
        });
    }

    // ---------------------------------------------------------------------

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

    // ---------------------------------------------------------------------

    std::pair<std::stack<char>, serialization::fragment_serialize_size_format>
    tree_navigator::write_indexed(std::span<const char> buffer, uint32_t alloc) {
        std::vector<chunk_collection*> same_amount_addresses_free;

        uint16_t addresses_free_max{};
        for(const auto& col:chunk_collections){
            uint16_t free_adr = col.first->free();
            if(free_adr > addresses_free_max)
                addresses_free_max = free_adr;
        }

        for(const auto& col:chunk_collections){
            if(col.first->free() == addresses_free_max){
                same_amount_addresses_free.push_back(col.first);
            }
        }

        //write to same amount_addresses_free where the alloc is most distant from the average fragment size

        auto min_size_of_most_free_addresses =
                *std::min_element(same_amount_addresses_free.begin(),same_amount_addresses_free.end(),
                                 [](const auto& a,const auto& b){
            return a->size() < b->size();
        });

    }

    // ---------------------------------------------------------------------

    std::pair<std::vector<char>, serialization::fragment_serialize_size_format>
    tree_navigator::read_indexed(const std::stack<char>& at) {

    }

    // ---------------------------------------------------------------------

    std::pair<std::stack<char>, std::vector<serialization::fragment_serialize_size_format>>
    tree_navigator::write_indexed_multi(const std::vector<std::span<const char>> &buffer) {

    }

    // ---------------------------------------------------------------------

    std::vector<std::pair<std::vector<char>, serialization::fragment_serialize_size_format>>
    tree_navigator::read_indexed_multi(const std::vector<std::stack<char>> &at) {

    }

    // ---------------------------------------------------------------------

    void tree_navigator::remove(uint8_t at) {

    }

    // ---------------------------------------------------------------------

    uint16_t tree_navigator::count() {

    }

    // ---------------------------------------------------------------------

    std::size_t tree_navigator::size() {

    }

    // ---------------------------------------------------------------------

    std::size_t tree_navigator::level_size() {

    }

    // ---------------------------------------------------------------------

    std::size_t tree_navigator::content_size() {

    }

    // ---------------------------------------------------------------------

    std::size_t tree_navigator::content_level_size() {

    }

    // ---------------------------------------------------------------------

    uint64_t tree_navigator::free() {

    }

    // ---------------------------------------------------------------------

    uint64_t tree_navigator::free_space() {

    }

    // ---------------------------------------------------------------------

    std::filesystem::path tree_navigator::getRoot() {

    }

    // ---------------------------------------------------------------------

} // namespace uh::io