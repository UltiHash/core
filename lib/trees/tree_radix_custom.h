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
        std::shared_mutex local_m{};
    public:
        tree_radix_custom();;

        std::list<tree_radix_custom*> add(const char* bin, std::size_t len, std::list<tree_radix_custom*> enlist = std::list<tree_radix_custom*>{});

        tree_radix_custom* copy();

        tree_radix_custom* copy_recursive();

        void destroy_recursive();

        void destroy_recursive(char sub);

        void insert(tree_radix_custom* in){
            std::list<std::tuple<tree_radix_custom*,unsigned char>> concat_string;
            concat_string.emplace_back(in,0);
            while(!concat_string.empty()){
                bool has_children = false;
                for(unsigned char &i=std::get<1>(concat_string.back()); i<(unsigned char)N;i++){
                    if(std::get<0>(concat_string.back())->children[i] != nullptr){
                        has_children=true;
                        concat_string.emplace_back(std::get<0>(concat_string.back())->children[i],0);
                        break;
                    }
                }
                if(!has_children){
                    std::size_t concat_size{},start_step{};
                    std::for_each(concat_string.cbegin(),concat_string.cend(),[&concat_size](auto in){
                        concat_size+=std::get<0>(in) -> length;
                    });
                    char* concat_sequence = (char*) std::malloc(concat_size * sizeof(char));
                    std::for_each(concat_string.cbegin(),concat_string.cend(),[&concat_sequence,&start_step](auto in){
                        std::memcpy(concat_sequence+start_step,std::get<0>(in) -> data,std::get<0>(in) -> length);
                        start_step+=std::get<0>(in) -> length;
                    });
                    (void) add(concat_sequence,concat_size);
                    std::free(concat_sequence);
                    concat_string.pop_back();
                }
            }
        }

        ~tree_radix_custom(){
            std::free(data);
        }
        //TODO: insert/merge radix tree and search and check search method and prefix pointer
    };
}

#endif //UHLIBCOMMON_TREE_RADIX_CUSTOM_H
