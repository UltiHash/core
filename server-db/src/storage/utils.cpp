#include "utils.h"


namespace uh::dbn::storage {

// ---------------------------------------------------------------------

bool maybe_write_data_to_filepath(const uh::protocol::blob &some_data,
                                                    const std::filesystem::path &filepath){
    if(std::filesystem::exists(filepath)) {
        INFO << "File already exists: " << filepath;
        return false;
    }

    util::temp_file tmp(filepath.parent_path());

    DEBUG << "Block written to " << filepath;
    tmp.write(some_data.data(), some_data.size());

    tmp.release_to(filepath);
    return true;
}

// ---------------------------------------------------------------------

uh::protocol::blob sha512(const uh::protocol::blob& some_data){
    DEBUG << "Beginning hash func";

    unsigned char result[SHA512_DIGEST_LENGTH];
    SHA512((unsigned char*) some_data.data(), some_data.size(), result);
    DEBUG << "sha512 calculated";


    //Original code//
    //char result_char[SHA512_DIGEST_LENGTH];
    //for(int i=0; i<SHA512_DIGEST_LENGTH; i++)
    //    result_char[i] = static_cast<char>(result[i]);
    //uh::protocol::blob hash_vec(std::begin(result_char), std::end(result_char));
    ////////////////

    //Max suggestion//
    uh::protocol::blob hash_vec(SHA512_DIGEST_LENGTH);
    memcpy(&hash_vec[0], &result[0], SHA512_DIGEST_LENGTH);
    ////////////////

    return hash_vec;
}

// ---------------------------------------------------------------------

size_t get_dir_size(const std::string& root_dir){

    size_t size=0;

    for(std::filesystem::recursive_directory_iterator it(root_dir);
        it!=std::filesystem::recursive_directory_iterator();
        ++it){
        if(!std::filesystem::is_directory(*it)){
            if(std::filesystem::exists(*it)){
                try{
                    size+=std::filesystem::file_size(*it);
                }
                catch(std::exception& e){
                    ERROR << e.what();
                    THROW(util::exception, e.what());
                }
            }
        }
    }

    return size;
}

// ---------------------------------------------------------------------

size_t max_configurable_capacity(const std::filesystem::path& dir)
{
    std::error_code ec;
    const std::filesystem::space_info si = std::filesystem::space(dir, ec);
    //si.capacity  - total size of the filesystem, in bytes
    //si.free      - free space on the filesystem, in bytes
    //si.available - free space available to a non-privileged process (may be equal or less than free)
    //
    //So, the maximum configurable capacity is the si.available + what already in used by the database:
    //
    //                         si.available + get_dir_size()
    //
    //For example: We start a new db node with a configured capacity of 1TB and a si.available of 1TB;
    //             We store 0.3 TB and the node is shut down.
    //             We start again the node, now si.available has dropped to 0.7 TB, but we still want 
    //                  to see it as a db of 1TB, thus we configure it as 1TB, since our data is
    //                  still there.
    return static_cast<size_t>(si.available) + get_dir_size(dir);
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::storage
