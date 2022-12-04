#include "storage_backend.h"
#include <fstream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <logging/logging_boost.h>

using namespace uh::dbn;
namespace fs = boost::filesystem;


std::vector<char> dump_storage::write_block(const std::vector<char>& some_data){

    std::string hash_str = "ThisIsTheHashKey";
    std::vector<char> hash_vec(hash_str.begin(), hash_str.end());


    fs::path filepath = this->m_config.db_file_fullpath;
    std::ofstream outfile(filepath, std::ios::out | std::ios::binary);
    if(!outfile.is_open()) {
        std::string msg("Error opening file: " + filepath.string());
        throw std::ofstream::failure(msg);
    }
    outfile.write(some_data.data(), some_data.size());

    DEBUG << "Block written to " << filepath;

    return hash_vec;
}

std::vector<char> dump_storage::read_block(const std::vector<char>& some_hash){

    fs::path filepath = this->m_config.db_file_fullpath;
    std::ifstream infile(filepath, std::ios::binary);

    if(!infile.is_open()) {
        std::string msg("Error opening file: " + filepath.string());
        throw std::ifstream::failure(msg);
    }

    std::vector<char> buffer(std::istreambuf_iterator<char>(infile), {});

    return buffer;
}
