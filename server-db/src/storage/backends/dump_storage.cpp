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

void dump_storage::start(){

    INFO << "--- Storage backend initialized --- " << std::filesystem::absolute(this->m_root);
    INFO << "        backend type   : " << backend_type();
    INFO << "        root diretcory : " << std::filesystem::absolute(this->m_root);
    INFO << "        space allocated: " << allocated_space();
    INFO << "        space available: " << free_space();
    INFO << "        space consumed : " << used_space();
}

// ---------------------------------------------------------------------

uh::protocol::blob dump_storage::write_chunk(const uh::protocol::blob& some_data){

    uh::protocol::blob hash_blob = this->hashing_function(some_data);
    
    // DEBUG
    // std::string printable_blob(hash_blob.begin(), hash_blob.end());
    // DEBUG << "hash string (should look like binary rubish):" << printable_blob;

    std::filesystem::path filepath = this->get_filepath_from_hash(hash_blob);

    //TODO check that there is available space before writing
    if(m_free < size(some_data)){
        THROW(util::exception, "Not enough space in node.");
    }


    if(maybe_write_data_to_filepath(some_data, filepath)){
        INFO << "Data chunk written to " << filepath;
        this->update_space_consumption();
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


void dump_storage::update_space_consumption(){
    m_used = get_dir_size(m_root);
    m_free = m_alloc - m_used;

    if(m_free <= 0){
        THROW(util::exception, "database node is full");
    }
    else if(m_free < 0.1 * m_alloc){
        WARNING << "DB NODE ALMOST FULL. CURRENTLY USED: " << used_space_percentage() << "%";
    }
}


} //namespace uh::dbn::storage