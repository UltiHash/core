//
// Created by benjamin-elias on 19.12.22.
//

#ifndef UHLIBCOMMON_TREE_STORAGE_H
#define UHLIBCOMMON_TREE_STORAGE_H

#include "conceptTypes/conceptTypes.h"
#include "logging/logging_boost.h"
#include <filesystem>
#include "boost/algorithm/hex.hpp"

namespace uh::trees{
#define N 256
#define STORE_MAX (unsigned int) std::numeric_limits<unsigned int>::max()
#define STORE_HARD_LIMIT (unsigned long) (std::numeric_limits<unsigned int>::max() * 2)
    typedef struct tree_storage tree_storage;

    struct tree_storage {
    protected:
        //every file storage level contains a maximum of 256 storage chunks and 256 folders to deeper levels
        std::size_t size[N]{};
        tree_storage* children[N]{};//deeper tree storage blocks and folders
        //radix_tree* block_indexes[N]{};
        std::filesystem::path combined_path{};
    public:
        tree_storage() {
            for(auto & i1 : size){
                i1 = 0;
            }
            for(auto & i : children){
                i = nullptr;
            }
        }

        explicit tree_storage(const std::filesystem::path& root):tree_storage(){
            //expected are 4 bytes that mimic hexadecimal string representation
            std::string parent_name = root.filename().string();
            bool valid_root=parent_name.size()==4 and root.extension().string().empty();
            if(valid_root)for(const char &i:parent_name){
                    if(!(('0'<= i and i <= '9')||('A'<= i and i <= 'F'))){
                        valid_root=false;
                        break;
                    }
                }
            combined_path = root;
            if(!valid_root){
                combined_path /= "/0000";
            }
            std::filesystem::create_directories(combined_path);
        }

        tree_storage * child(char i){
            return children[(unsigned char)i];
        }
        
    };
}

#endif //UHLIBCOMMON_TREE_STORAGE_H
