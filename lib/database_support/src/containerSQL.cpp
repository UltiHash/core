//
// Created by benjamin-elias on 29.06.21.
//

#include "containerSQL.h"

std::string containerSQL::currentSchema() {
    const std::lock_guard<std::recursive_mutex> lock(sqlMutex_single);
    return schem;
}

std::string containerSQL::currentTableName() {
    const std::lock_guard<std::recursive_mutex> lock(sqlMutex_single);
    return tab;
}

bool containerSQL::isOpen(const std::string &table) {
    const std::lock_guard<std::recursive_mutex> lock(sqlMutex_single);
    return open and tab==table;
}

