#include "rabin_fp.h"
#include <logging/logging_boost.h>
#include <util/exception.h>
#include <vector>
#include <iostream>


namespace uh::chunking
{

// ---------------------------------------------------------------------

rabin_fp::rabin_fp(const rabin_fp_config& config ,io::device& dev)
    : m_dev(dev),
      m_chunk_size(config.read_buf_size),
      m_buffer(config.read_buf_size),
      m_block(nullptr),
      m_chunk(nullptr),
      m_update_chunk(false),
      m_finished(false)
{
    static int __rabin_init_result = initialize_rabin_polynomial_defaults();

    if(!__rabin_init_result){
        throw(std::runtime_error("Error initializing Rabin fingerprints"));
    }

    INFO << "--- Storage backend initialized --- ";
    INFO << "        chunking strategy : " << chunker_type();
}

// ---------------------------------------------------------------------

rabin_fp::~rabin_fp(){
    free_chunk_data(m_block);
    free_rabin_fingerprint_list(m_block->head);
    free(m_block);
}

// ---------------------------------------------------------------------

size_t rabin_fp::refill_buffer()
{
    size_t bytes_read=m_dev.read({m_buffer.data(), m_buffer.size()});

    if(!bytes_read)
        return bytes_read;

    m_block=read_rabin_block(m_buffer.data(), bytes_read, m_block);

    if(!m_block)
        THROW(util::exception, "Out of memory to read new rabin block");

    if(!m_chunk){
        m_chunk=m_block->head;
    }

    return bytes_read;
}

std::span<char> rabin_fp::next_chunk()
{
    size_t bytes_read = 0;
    FILE *myfile = fopen("/home/juan/Repos/core/chunkdatafile_c", "a"); // <--- JM DEBUG

    // This clause should only run for the beginning of a file.
    if(!m_chunk){
        bytes_read = refill_buffer();
        if(!bytes_read){
            return {};
        }
    }

    // Many chunks are filled only partially by the end of a buffer and then fully filled by the beginning of the next buffer.
    // When chunks are only filled partially, their "next_polynomial" attribute is not set, so the above clause is engaged
    // and the chunk is completed.
    // The situation is different at the end of a file, since the file can end without a rabin fingerprint being completed.
    // In such a case, the above clause runs refilling the buffer (and m_block), but next_polynomial may still be null. Thus,
    // upon calling the function once again, the clause runs again, returning the last chunk of the file (even if it is an
    // incomplete polynomial).
    if(!m_chunk->next_polynomial){
        bytes_read = refill_buffer();
        if(!bytes_read){
            if(!m_finished){
                m_finished = true;
                fwrite(m_chunk->chunk_data, sizeof(char), m_chunk->length,myfile);// <--- JM DEBUG
                fclose(myfile);// <--- JM DEBUG
                return {m_chunk->chunk_data, m_chunk->length};
            }
            else{
                return {};
            }
        }
        return next_chunk();
    }

    // Base case: return the current chunk
    if(!m_update_chunk)
    {
        m_update_chunk = !m_update_chunk;
        fwrite(m_chunk->chunk_data, sizeof(char), m_chunk->length,myfile);// <--- JM DEBUG
        fclose(myfile);// <--- JM DEBUG
        return {m_chunk->chunk_data, m_chunk->length};
    }

    // Only update the chunk (It is guaranteed that m_chunk and m_chunk->next_polynomial are not null).
    // If the new chunk is a special case, it will be dealt with in the above clauses by calling the function recursively.
    else
    {
        m_update_chunk = !m_update_chunk;
        m_chunk = m_chunk->next_polynomial;
        return next_chunk() ;
    }
}

// ---------------------------------------------------------------------

} // namespace uh::client::chunking
