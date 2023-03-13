#ifndef SERVER_DB_STORAGE_BACKEND_H
#define SERVER_DB_STORAGE_BACKEND_H


#include "utils.h"


namespace uh::dbn::storage {

    class storage_backend {
    public:
        virtual ~storage_backend() = default;

        virtual void start() = 0;

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
        virtual uh::protocol::block_meta_data write_block(const uh::protocol::blob &data) = 0;

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
        virtual std::unique_ptr<io::device> read_block(const uh::protocol::blob& hash) = 0;

        /**
         * Return free space in this storage back-end in bytes.
         */
        virtual size_t free_space() = 0;

        /**
         * Return space already in use in this storage back-end in bytes.
         */
        virtual size_t used_space() = 0;

        /**
         * Return total space allocated in this storage back-end in bytes.
         */
        virtual size_t allocated_space() = 0;

        /**
         * Return the name of the storage backend type as a std::string.
         */
        virtual std::string backend_type() = 0;
    };

// ---------------------------------------------------------------------


} // namespace uh::dbn::storage

#endif
