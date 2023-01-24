#ifndef SERVER_DB_STORAGE_BACKEND_H
#define SERVER_DB_STORAGE_BACKEND_H


#include "utils.h"


namespace uh::dbn::storage {

    class storage_backend {
    public:
        virtual ~storage_backend() = default;


        /**
         * Write a data chunk and return it's hash.
         *
         * This function stores the chunk contained in `data` in an permanent
         * storage and returns a hash that can be used to retrieve the data
         * at a later point of time.
         *
         * @return the hash
         * @throw may throw any derivative of exception on error
         */
        virtual uh::protocol::blob write_chunk(const uh::protocol::blob &data) = 0;

        /**
         * Read a data chunk identified by it's hash from the storage.
         *
         * This function reads the chunk identified by `hash` from the storage and
         * returns it in a vector. If the chunk is not known the function will
         * throw.
         *
         * @return the data chunk
         * @throw may throw any derivative of exception on error
         */
        virtual uh::protocol::blob read_chunk(const uh::protocol::blob &hash) = 0;


        size_t free_space(){return m_total - m_used;}


        size_t used_space(){return m_used;}


        size_t total_space(){return m_total;}

    private:

        /**
         * Given a chunk of data, return its hash
         *
         * This function computes a hash string givena chunk of binary data
         *
         * @return the hash
         * @throw may throw any derivative of exception on error
         */
        virtual uh::protocol::blob hashing_function(const uh::protocol::blob &data) = 0;


        /**
         * Given a hash string as a uh::protocol::blob, return the file path
         * to the corresponding data chunk
         * @param hash: the hash as a uh::protocol::blob
         * @return a file path as a boost::filesystem::path
         */
        virtual std::filesystem::path get_filepath_from_hash(const uh::protocol::blob &hash) = 0;

    protected:

        std::filesystem::path m_root; //root path of the db
        size_t m_total; //total space
        size_t m_free;  //free space
        size_t m_used;  //used space
    };

// ---------------------------------------------------------------------


} // namespace uh::dbn::storage

#endif
