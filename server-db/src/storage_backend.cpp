#include "storage_backend.h"
#include <logging/logging_boost.h>

#include <util/temp_file.h>
#include <fstream>

using namespace uh::dbn;

// ---------------------------------------------------------------------

bool uh::dbn::maybe_write_data_to_filepath(const std::vector<char> &some_data, const std::filesystem::path &filepath){
    if(std::filesystem::exists(filepath)) {
        INFO << "Data block already exists at " << filepath;
        return false;
    }

    util::temp_file tmp(filepath.parent_path());

    DEBUG << "Block written to " << filepath;
    tmp.write(some_data.data(), some_data.size());

    tmp.release_to(filepath);
    return true;
}

// ---------------------------------------------------------------------

std::vector<char> uh::dbn::sha512(const std::vector<char>& some_data){
    DEBUG << "Beginning hash func";

    unsigned char result[SHA512_DIGEST_LENGTH];
    SHA512((unsigned char*) some_data.data(), some_data.size(), result);
    DEBUG << "sha512 calculated";

    // What is a better way of accomplishing this?
    ///////////////////////////////////////////////////////////////////////////
    char result_char[SHA512_DIGEST_LENGTH];
    for(int i=0; i<SHA512_DIGEST_LENGTH; i++)
        result_char[i] = static_cast<char>(result[i]);
    std::vector<char> hash_vec(std::begin(result_char), std::end(result_char));
    ///////////////////////////////////////////////////////////////////////////

    return hash_vec;
}

// ---------------------------------------------------------------------

std::filesystem::path dump_storage::get_filepath_from_hash(const std::vector<char> &hash){

    std::string hash_string = to_hex_string(hash.begin(), hash.end());

    DEBUG << "---------------------------";
    DEBUG << "Here is the formated sha512 string";
    DEBUG << hash_string;
    DEBUG << "---------------------------";


    std::filesystem::path filepath = this->m_config.db_root / hash_string;
    return filepath;
}

// ---------------------------------------------------------------------

std::vector<char> dump_storage::hashing_function(const std::vector<char> &data) {
    return uh::dbn::sha512(data);
}

// ---------------------------------------------------------------------

std::vector<char> dump_storage::write_block(const std::vector<char>& some_data){

    std::vector<char> hash_vec = this->hashing_function(some_data);
    std::string hash_string(hash_vec.begin(), hash_vec.end());

    DEBUG << "hash string (should look like binary rubish):" << hash_string;

    std::filesystem::path filepath = this->get_filepath_from_hash(hash_vec);

    if(maybe_write_data_to_filepath(some_data, filepath)){
        INFO << "Data block written to " << filepath;
    }
    else{
        INFO << "Skipped writing to existing filepath: " << filepath;
    };

    return  hash_vec;
}

std::vector<char> dump_storage::read_block(const std::vector<char>& some_hash){

    std::string hash_string(some_hash.begin(), some_hash.end());

    std::filesystem::path filepath = this->get_filepath_from_hash(some_hash);
    std::ifstream infile(filepath, std::ios::binary);

    if(!infile.is_open()) {
        std::string msg("Error opening file: " + filepath.string());
        throw std::ifstream::failure(msg);
    }

    std::vector<char> buffer(std::istreambuf_iterator<char>(infile), {});

    return buffer;
}
