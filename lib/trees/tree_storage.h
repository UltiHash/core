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
        tree_storage* children[N]{};//deeper tree storage blocks and folders
        std::size_t size[N]{};
        unsigned char root_name[2]{};
    public:
        tree_storage() {
            for(auto & i1 : size){
                i1 = 0;
            }
            for(auto & i : children){
                i = nullptr;
            }
        }

        void init(const std::filesystem::path& root,bool is_root=true){
            if(!is_root){
                //expected are 4 bytes that mimic hexadecimal string representation
                char* parent_name = const_cast<char *>(root.parent_path().filename().c_str());
                boost::algorithm::unhex(parent_name,parent_name+4,root_name);//FF,FF to {BIN,BIN}
            }
            else{
                root_name[0]=0;
                root_name[1]=0;
            }

        }

        tree_storage * child(char i){
            return children[(unsigned char)i];
        }
        
    };
}

#endif //UHLIBCOMMON_TREE_STORAGE_H
