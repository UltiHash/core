#ifndef SERVER_DB_STORAGE_BACKEND_H
#define SERVER_DB_STORAGE_BACKEND_H


#include "utils.h"


namespace uh::dbn::storage {

    class storage_backend {
    public:
        virtual ~storage_backend() = default;

        virtual void start() = 0;

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
