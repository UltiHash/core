//
// Created by benjamin-elias on 09.05.22.
//

#ifndef SCHOOL_PROJECT_CONFIG_FILES_H
#define SCHOOL_PROJECT_CONFIG_FILES_H

#include <conceptTypes/conceptTypes.h>
#include <logging/logging_boost.h>
#include <logging/custom_exceptions.h>

class config_files {

    std::filesystem::path at;
public:
    [[maybe_unused]] void run();

    std::vector<std::string> read();

    explicit config_files(std::filesystem::path p);
};


#endif //SCHOOL_PROJECT_CONFIG_FILES_H
