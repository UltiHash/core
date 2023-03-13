#ifndef SERVER_DB_STORAGE_BACKEND_H
#define SERVER_DB_STORAGE_BACKEND_H

#include <protocol/server.h>
#include "utils.h"


namespace uh::dbn::storage {


    class backend {
    public:
        virtual ~backend() = default;

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
         * Reserve data storage of given `size` and return an allocation for it.
         */
        virtual std::unique_ptr<uh::protocol::allocation> allocate(std::size_t size) = 0;
    };

// ---------------------------------------------------------------------


} // namespace uh::dbn::storage

#endif
