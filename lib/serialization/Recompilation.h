//
// Created by tariq on 5/11/22.
//

#pragma once
#include <utility>

#include "Data.h"

//TODO: save global current path before Recompilation

class Recompilation final : virtual public std::fstream {
private:
    std::string db_config_file;
    std::list<Data> data; // just stream data; save data blocks directly to the database, while return the prefix and hash blocks to the compilation class who saves them file/folder by file/folder.
    enum Datatype {
        notexist, directories, files, unknown
    };
    std::vector<Datatype> treeTypes;
    bool overwrite = true;
    bool encodingMode = false;
    std::map<std::string, bool> flag_check{};
    std::list<std::tuple<std::filesystem::path,std::filesystem::path>> input_commands{};
    bool multimode{};

    static std::filesystem::path fileNameCount(std::filesystem::path filename);

protected:
    //file tree to recompilation file
    bool encode();
    //recompilation file to file tree
    bool decode();

public:
    Recompilation(const std::string& db_config, bool encodeBool, std::list<std::string> input,std::map<std::string, bool> flags);

    ~Recompilation() final;

    static bool valid();

    std::vector<std::filesystem::path> ls();

    bool cd();
};
