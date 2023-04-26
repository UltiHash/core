#ifndef SERVER_DB_STORAGE_UTILS_H
#define SERVER_DB_STORAGE_UTILS_H

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <fstream>
#include <logging/logging_boost.h>
#include <openssl/sha.h>
#include <string>
#include <io/temp_file.h>
#include <vector>
#include <util/exception.h>

//TODO
//This include is only needed because of the blob definition; otherwise is irrelevant.
//Can we define blob at a larget scope?
#include <protocol/client_pool.h>

namespace uh::dbn::storage {
// ---------------------------------------------------------------------

    template <typename Iterator>
    std::string to_hex_string(Iterator begin, Iterator end)
    {
        // A home-made boost-free implementation:
        //
        // std::stringstream ss;
        // while(begin != end)
        // {
        //     ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(*begin));
        //     begin++;
        // }
        // hash_string = ss.str();
        // return hash_string;

        std::string legible_hash;
        boost::algorithm::hex(begin, end, std::back_inserter(legible_hash));
        boost::algorithm::to_lower(legible_hash);
        return legible_hash;
    }

// ---------------------------------------------------------------------

    bool maybe_write_data_to_filepath(const uh::protocol::blob &some_data, const std::filesystem::path &filepath);

// ---------------------------------------------------------------------

    size_t get_dir_size(const std::string& root_dir);

// ---------------------------------------------------------------------

    size_t max_configurable_capacity(const std::filesystem::path& dir);

// ---------------------------------------------------------------------
} // uh::dbn::storage

#endif
