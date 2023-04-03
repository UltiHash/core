#ifndef CLIENT_FILE_CHUNKING_H
#define CLIENT_FILE_CHUNKING_H

//#include "utils.h"
#include <uhv/f_meta_data.h>
#include <protocol/client_pool.h>
#include <util/exception.h>


namespace uh::client::chunking {

    class file_chunker {
    public:
        virtual ~file_chunker() = default;

        virtual void start() = 0;

        /**
         * Split a datafile into chunks and return them.
         *
         * @return a vector of chunks
         * @throw may throw any derivative of exception on error
         */
        virtual std::vector<uh::protocol::blob> chunk_files(std::unique_ptr<uhv::f_meta_data>&) = 0;

        /**
         * Return the name of the chunking strategy as a std::string.
         */
        virtual std::string chunking_type() = 0;
    };

// ---------------------------------------------------------------------

} // namespace uh::client::chunking

#endif
