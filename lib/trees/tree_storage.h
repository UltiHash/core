//
// Created by benjamin-elias on 19.12.22.
//

#ifndef UHLIBCOMMON_TREE_STORAGE_H
#define UHLIBCOMMON_TREE_STORAGE_H

#include "conceptTypes/conceptTypes.h"
#include "logging/logging_boost.h"
#include <shared_mutex>

namespace uh::trees{
#define N 256
    typedef struct tree_storage tree_storage;

    struct tree_storage {
    protected:
        tree_storage* children[N]{};//deeper tree storage blocks and folders
        std::size_t size[N]{};
    public:
        tree_storage() {
            for(auto & i1 : size){
                i1 = 0;
            }
            for(auto & i : children){
                i = nullptr;
            }
        }

        tree_storage * child(char i){
            return children[(unsigned char)i];
        }
        
    };
}

#endif //UHLIBCOMMON_TREE_STORAGE_H
