//
// Created by benjamin-elias on 28.05.23.
//

#include "tree_navigator.h"

#include "boost/algorithm/hex.hpp"

#include <filesystem>

namespace uh::io {

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

} // namespace uh::io