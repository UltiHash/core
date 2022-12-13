//
// Created by benjamin-elias on 09.05.22.
//

#include "config_files.h"

void config_files::run() {
    std::vector<std::string> commands=read();
    auto getcwd = std::filesystem::current_path();
    for(auto &c:commands){
        (void) std::system(c.c_str());
        INFO << "At " << getcwd.string() << " Command: " << c << std::endl;
    }
}

std::vector<std::string> config_files::read() {
    if (!std::filesystem::exists(at)){
        THROW(uh::util::exception, "Configuration file at "+at.string()+" could not be found!");
    }
    if(std::filesystem::is_empty(at)){
        THROW(uh::util::exception, "Configuration file at "+at.string()+" was empty!");
    }
    std::fstream file(at.c_str());
    std::string str;
    std::vector<std::string> lines_in_file;
    while(getline(file, str)){
        lines_in_file.push_back(str);
    }
    file.close();
    INFO << "Successfully reader Database configuration file at " << at.c_str() << std::endl ;
    return lines_in_file;
}

config_files::config_files(std::filesystem::path p) {
    at=std::move(p);
}