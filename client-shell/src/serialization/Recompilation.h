//
// Created by tariq on 5/11/22.
//

#pragma once
#include <utility>

#include "Data.h"
#include "recomp_tree.h"

//TODO: save global current path before Recompilation
using namespace uh::recomp;
class Recompilation final : virtual public std::fstream {
private:
    std::list<Data> data; // just stream data; save data blocks directly to the database, while return the prefix and hash blocks to the compilation class who saves them file/folder by file/folder.
    enum Datatype {
        notexist, directories, files, unknown
    };
    std::vector<Datatype> treeTypes;
    bool overwrite = true;
    unsigned char recomp_Mode{};
    std::map<std::string, bool> flag_check{};
    std::list<std::tuple<std::filesystem::path,std::filesystem::path>> input_commands{};
    bool multimode{};
    static std::filesystem::path fileNameCount(std::filesystem::path filename);
    uh::protocol::client& m_client;
    std::shared_ptr<treeNode<std::string,std::uint64_t>> rootShrPtr;

protected:
    //file tree to recompilation file
    bool encode();
    //recompilation file to file tree
    bool decode();

public:
    Recompilation(unsigned char in_type, std::list<std::string> input,std::map<std::string, bool> flags, uh::protocol::client& client);

    ~Recompilation() final;

    static bool valid();

    bool ls();
    bool cd();
};
