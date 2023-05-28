//
// Created by benjamin-elias on 28.05.23.
//

#ifndef CORE_TREE_NAVIGATOR_H
#define CORE_TREE_NAVIGATOR_H

#include "io/chunk_collection.h"

#include <vector>

namespace uh::io {

    class tree_navigator {

        explicit tree_navigator(const std::filesystem::path& root);

    private:
        std::vector<std::pair<tree_navigator*,uint8_t>> sub_trees;
        std::vector<std::pair<chunk_collection*,uint8_t>> chunk_collections;

        std::filesystem::path root;
    };

} // namespace uh::io

#endif //CORE_TREE_NAVIGATOR_H
