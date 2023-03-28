#ifndef CLIENT_CHUNKING_FIXED_SIZE_H
#define CLIENT_CHUNKING_FIXED_SIZE_H

#include <chunking/file_chunker.h>
#include <protocol/client_pool.h>


namespace uh::client::chunking {

class fixed_size_chunker : public file_chunker {
    public:
        fixed_size_chunker(size_t chunk_size_in_bytes):
        m_chunk_size(chunk_size_in_bytes)
        {}

        virtual void start() override;

        /**
         * Split a datafile into chunks and return them.
         *
         * @return a vector of chunks
         * @throw may throw any derivative of exception on error
         */
        virtual std::vector<uh::protocol::blob> chunk_files(std::unique_ptr<common::f_meta_data>&) override;

        /**
         * Return the name of the chunking strategy as a std::string.
         */
        virtual std::string chunking_type() override {return std::string(m_type);}
        
        size_t chunk_size() {return m_chunk_size;}
    
    protected:

        constexpr static std::string_view m_type = "FixedSize";
        size_t m_chunk_size = 0;
    };

} // namespace uh::dbn::storage

#endif
