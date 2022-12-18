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

        ~tree_radix_custom(){
            std::free(data);
        }
        //TODO: insert/merge radix tree and search and check search method and prefix pointer
    };
}

#endif //UHLIBCOMMON_TREE_RADIX_CUSTOM_H
