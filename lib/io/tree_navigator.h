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
        std::vector<tree_navigator*> sub_trees;
        std::vector<chunk_collection> chunk_collections;
    };

} // namespace uh::io

#endif //CORE_TREE_NAVIGATOR_H
