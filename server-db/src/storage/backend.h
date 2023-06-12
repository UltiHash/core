#ifndef SERVER_DB_STORAGE_BACKEND_H
#define SERVER_DB_STORAGE_BACKEND_H

#include <protocol/server.h>
#include "utils.h"


namespace uh::dbn::storage {


    class backend {
    public:
        virtual ~backend() = default;

        virtual void start() = 0;

        virtual void stop() = 0;

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
        virtual std::unique_ptr<io::data_generator> read_block(const std::span <char>& hash) = 0;

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

        /**
         * Writes the data to the storage backend and returns its hash and effective size
         */
        virtual std::pair <std::size_t, std::vector <char>> write_block (const std::span <const char>& data) = 0;

    };

// ---------------------------------------------------------------------


} // namespace uh::dbn::storage

#endif
