//
// Created by benjamin-elias on 09.05.22.
//

#ifndef SCHOOL_PROJECT_CONFIG_FILES_H
#define SCHOOL_PROJECT_CONFIG_FILES_H

#include "conceptTypes.h"
#include "logging_boost.h"
#include "custom_exceptions.h"

class config_files {

    std::filesystem::path at;
public:
    [[maybe_unused]] void run();

    std::vector<std::string> read();

    explicit config_files(std::filesystem::path p);
};


#endif //SCHOOL_PROJECT_CONFIG_FILES_H
