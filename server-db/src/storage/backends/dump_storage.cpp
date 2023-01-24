#include "dump_storage.h"

namespace uh::dbn::storage {


std::filesystem::path dump_storage::get_filepath_from_hash(const uh::protocol::blob &hash){

    std::string hash_string = to_hex_string(hash.begin(), hash.end());

    DEBUG << "Here is the formated sha512 string " << hash_string;

    std::filesystem::path filepath = this->m_root / hash_string;
    return filepath;
}

// ---------------------------------------------------------------------

uh::protocol::blob dump_storage::hashing_function(const uh::protocol::blob &data) {
    return uh::dbn::storage::sha512(data);
}

// ---------------------------------------------------------------------

uh::protocol::blob dump_storage::write_chunk(const uh::protocol::blob& some_data){

    uh::protocol::blob hash_blob = this->hashing_function(some_data);
    
    // DEBUG
    // std::string printable_blob(hash_blob.begin(), hash_blob.end());
    // DEBUG << "hash string (should look like binary rubish):" << printable_blob;

    std::filesystem::path filepath = this->get_filepath_from_hash(hash_blob);

    if(maybe_write_data_to_filepath(some_data, filepath)){
        INFO << "Data chunk written to " << filepath;
    }
    else{
        INFO << "Skipped writing to existing filepath: " << filepath;
    };

    return  hash_blob;
}

uh::protocol::blob dump_storage::read_chunk(const uh::protocol::blob& some_hash){

    std::string hash_string(some_hash.begin(), some_hash.end());

    std::filesystem::path filepath = this->get_filepath_from_hash(some_hash);
    std::ifstream infile(filepath, std::ios::binary);

    if(!infile.is_open()) {
        std::string msg("Error opening file: " + filepath.string());
        throw std::ifstream::failure(msg);
    }

    uh::protocol::blob buffer(std::istreambuf_iterator<char>(infile), {});

    return buffer;
}

} //namespace uh::dbn::storage