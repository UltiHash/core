#include "dump_storage.h"

#include <io/file.h>


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

uh::protocol::block_meta_data dump_storage::write_block(const uh::protocol::blob& some_data){

    uh::protocol::blob hash_blob = this->hashing_function(some_data);

    // DEBUG
    // std::string printable_blob(hash_blob.begin(), hash_blob.end());
    // DEBUG << "hash string (should look like binary rubish):" << printable_blob;

    std::filesystem::path filepath = this->get_filepath_from_hash(hash_blob);

    if(m_free < size(some_data)){
        THROW(util::exception, "Not enough space in node.");
    }

    std::uint64_t effective_size;
    if(maybe_write_data_to_filepath(some_data, filepath)){
        INFO << "Data block written to " << filepath;
        effective_size = some_data.size();
        this->update_space_consumption();
    }
    else{
        INFO << "Skipped writing to existing filepath: " << filepath;
        effective_size = 0;
    };

    return  {
        .hash = std::move(hash_blob),
        .effective_size = effective_size,
    };
}

// ---------------------------------------------------------------------

std::unique_ptr<io::device> dump_storage::read_block(const uh::protocol::blob& hash) {

    std::string hash_string(hash.begin(), hash.end());

    std::filesystem::path filepath = this->get_filepath_from_hash(hash);
    return std::make_unique<io::file>(filepath);
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::protocol::allocation> dump_storage::allocate(std::size_t size)
{
    THROW(util::exception, "not implemented");
}

// ---------------------------------------------------------------------

void dump_storage::update_space_consumption(){
    m_used = get_dir_size(m_root);
    m_free = m_alloc - m_used;
    m_storage_metrics.alloc_space().Set(m_alloc);
    m_storage_metrics.free_space().Set(m_free);
    m_storage_metrics.used_space().Set(m_used);

    if(m_free <= 0){
        THROW(util::exception, "database node is full");
    }
    else if(m_free < 0.1 * m_alloc){
        WARNING << "DB NODE ALMOST FULL. CURRENTLY USED: " << used_space_percentage() << "%";
    }
}


} //namespace uh::dbn::storage
