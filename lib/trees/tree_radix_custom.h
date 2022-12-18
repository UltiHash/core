//
// Created by benjamin-elias on 17.12.22.
//

#ifndef UHLIBCOMMON_TREE_RADIX_CUSTOM_H
#define UHLIBCOMMON_TREE_RADIX_CUSTOM_H

#include "conceptTypes/conceptTypes.h"
#include "logging/logging_boost.h"
#include <shared_mutex>

namespace uh::util{
#define N 256
    typedef struct tree_radix_custom tree_radix_custom;

    struct tree_radix_custom {
    protected:
        tree_radix_custom* children[N]{};
        char* data{}; // Storing for printing purposes only
        std::size_t length{};
    public:
        tree_radix_custom();;

        std::list<tree_radix_custom*> add(const char* bin, std::size_t len, std::list<tree_radix_custom*> enlist = std::list<tree_radix_custom*>{});

        tree_radix_custom* copy();

        tree_radix_custom* copy_recursive();

        void destroy_recursive();

        void destroy_recursive(char sub);

        void insert(tree_radix_custom* in);

        bool has_children();

        std::tuple<std::list<tree_radix_custom*>,std::size_t> search(const char* bin, std::size_t len, std::tuple<std::list<tree_radix_custom*>,std::size_t> enlist = std::make_tuple(std::list<tree_radix_custom*>{},std::size_t{})){
            if(len>0){
                if(length == 0){
                    if(!has_children() || children[(unsigned char)bin[0]] == nullptr){
                        return enlist;
                    }
                    else{
                        std::get<0>(enlist).push_back(this);
                        return children[(unsigned char)bin[0]] -> search(bin,len,enlist);
                    }
                }
                else{

                }
            }
            else return enlist;
        }

        ~tree_radix_custom();
        //TODO: insert/merge radix tree and search and check search method and prefix pointer
    };
}

#endif //UHLIBCOMMON_TREE_RADIX_CUSTOM_H
