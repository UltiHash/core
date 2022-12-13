//
// Created by benjamin-elias on 17.06.22.
//

#include "Block.h"

std::size_t Block::size() const {
    return infoS.size();
}

std::string& Block::get() {
    return infoS;
}
