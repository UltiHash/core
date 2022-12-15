#ifndef SERVER_DB_STORAGE_BACKEND_H
#define SERVER_DB_STORAGE_BACKEND_H

#include <vector>
#include <string>
#include <boost/filesystem.hpp>
#include <openssl/sha.h>
#include <logging/logging_boost.h>

#include "db_config.h"


namespace fs = boost::filesystem;

namespace uh::dbn {

// ---------------------------------------------------------------------

    template <typename Iterator>
    std::string to_hex_string(Iterator begin, Iterator end);

    template <typename Iterator>
    std::string uh::dbn::to_hex_string(Iterator begin, Iterator end)
    {
        std::stringstream ss;
        while(begin != end)
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(*begin));
            begin++;
        }
        return ss.str();
    }

// ---------------------------------------------------------------------

    bool maybe_write_data_to_filepath(const std::vector<char> &some_data, const fs::path &filepath);


// ---------------------------------------------------------------------

    std::vector<char> sha512(const std::vector<char>& some_data);

// ---------------------------------------------------------------------

    class storage_backend {
    public:
        virtual ~storage_backend() = default;


        /**
         * Write a data block and return it's hash.
         *
         * This function stores the block contained in `data` in an permanent
         * storage and returns a hash that can be used to retrieve the data
         * at a later point of time.
         *
         * @return the hash
         * @throw may throw any derivative of exception on error
         */
        virtual std::vector<char> write_block(const std::vector<char> &data) = 0;

        /**
         * Read a data block identified by it's hash from the storage.
         *
         * This function reads the block identified by `hash` from the storage and
         * returns it in a vector. If the block is not known the function will
         * throw.
         *
         * @return the data block
         * @throw may throw any derivative of exception on error
         */
        virtual std::vector <char> read_block(const std::vector<char> &hash) = 0;

    private:

        /**
         * Given a block of data, return its hash
         *
         * This function computes a hash string givena block of binary data
         *
         * @return the hash
         * @throw may throw any derivative of exception on error
         */
        virtual std::vector <char> hashing_function(const std::vector<char> &data) = 0;


        /**
         * Given a hash string as a std::vector<char>, return the file path
         * to the corresponding data block
         * @param hash: the hash as a std::vector
         * @return a file path as a boost::filesystem::path
         */
        virtual fs::path get_filepath_from_hash(const std::vector<char> &hash) = 0;
    };

// ---------------------------------------------------------------------

//TODO:
//class storage_backend -->  class dump_storage
//class dump_storage
//class radix_tree_storage
//class etc_storage
//class postgress_storage

//TODO:
// Write unit test, generic, for any storage backend:
// Write block, read block, read non-existent block and receive error, write with error and throw exception.

    class dump_storage : public storage_backend {
    public:
        dump_storage(const db_config &some_config):
        m_config(some_config)
        {
            if(!(boost::filesystem::exists(some_config.db_root))) {
                std::string msg("Path does not exist: " + some_config.db_root.string());
                throw std::runtime_error(msg);
            }
        }

        /**
         * Write a data block and return it's hash.
         *
         * This function stores the block contained in `data` in an permanent
         * storage and returns a hash that can be used to retrieve the data
         * at a later point of time.
         *
         * @return the hash
         * @throw may throw any derivative of exception on error
         */
        virtual std::vector<char> write_block(const std::vector<char> &data) override;

        /**
         * Read a data block identified by it's hash from the storage.
         *
         * This function reads the block identified by `hash` from the storage and
         * returns it in a vector. If the block is not known the function will
         * throw.
         *
         * @return the data block
         * @throw may throw any derivative of exception on error
         */
        virtual std::vector <char> read_block(const std::vector<char> &hash) override;


    private:

        /**
         * Given a block of data, return its hash
         *
         * This function computes a hash string givena block of binary data
         *
         * @return the hash
         * @throw may throw any derivative of exception on error
         */
        virtual std::vector<char> hashing_function(const std::vector<char> &data) override;

        /**
         * Given a hash string as a std::vector<char>, return the file path
         * to the corresponding data block
         * @param hash: the hash as a std::vector
         * @return a file path as a boost::filesystem::path
         */
        virtual fs::path get_filepath_from_hash(const std::vector<char> &hash);

        db_config m_config;
    };

// ---------------------------------------------------------------------

} // namespace uh::dbn

#endif
