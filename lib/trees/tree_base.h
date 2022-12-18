//
// Created by benjamin-elias on 18.12.22.
//

#ifndef UHLIBCOMMON_TREE_BASE_H
#define UHLIBCOMMON_TREE_BASE_H


#include "conceptTypes/conceptTypes.h"
#include "logging/logging_boost.h"
#include <shared_mutex>

namespace uh::util{
#define N 256
    typedef struct tree_base tree_base;

    struct tree_base {
    protected:
        tree_base* children[N]{};
        std::shared_mutex local_m{};
    public:
        tree_base(){
            for(auto & i : children){
                i = nullptr;
            }
        };

        virtual tree_base* copy(){
            std::shared_lock lock(local_m);
            auto* tmp = (struct tree_base*) std::malloc(sizeof(struct tree_base));
            new (tmp) tree_base();
            std::memcpy(tmp->children,this->children,N * sizeof(tree_base*));
            return tmp;
        }

        virtual tree_base* copy_recursive(){
            auto* tmp = copy();
            std::shared_lock lock(local_m);
            for(unsigned char i=0;i<(unsigned char)N;i++){
                if(children[i] != nullptr){
                    tmp->children[i] = children[i]->copy_recursive();
                }
            }
            return tmp;
        }

        virtual void destroy_recursive(){
            std::shared_lock lock(local_m);
            for(auto & i : children){
                if(i != nullptr){
                    i -> destroy_recursive();
                    delete i;
                }
            }
        }

        virtual void destroy_recursive(char sub){
            if(children[sub] != nullptr){
                std::shared_lock lock(local_m);
                children[sub] -> destroy_recursive();
                std::free(children[sub]);
                children[sub] = nullptr;
            }
        }
    };
}


#endif //UHLIBCOMMON_TREE_BASE_H
