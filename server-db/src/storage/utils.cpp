#include "utils.h"


namespace uh::dbn::storage {

// ---------------------------------------------------------------------


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

} // namespace uh::dbn::storage
