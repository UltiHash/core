#include "rabin_fingerprints_chunker.h"
#include <logging/logging_boost.h>
#include <util/exception.h>
#include <vector>
#include <iostream>


namespace uh::chunking
{

// ---------------------------------------------------------------------

rabin_fingerprints_chunker::rabin_fingerprints_chunker(const rabin_fingerprints_config& config ,io::device& dev)
    : m_dev(dev),
      m_chunk_size(config.chunk_size),
      m_buffer(config.chunk_size),
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

rabin_fingerprints_chunker::~rabin_fingerprints_chunker(){
    free_chunk_data(m_block);
    free_rabin_fingerprint_list(m_block->head);
    free(m_block);
}

// ---------------------------------------------------------------------

size_t rabin_fingerprints_chunker::refill_buffer()
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

std::span<char> rabin_fingerprints_chunker::next_chunk()
{
    size_t bytes_read = 0;
    FILE *myfile = fopen("/home/juan/Repos/core/chunkdatafile_c", "a"); // <--- JM DEBUG

    if(!m_chunk){
        bytes_read = refill_buffer();
        if(!bytes_read){
            return {};
        }
        return next_chunk();
    }
    else if(!m_chunk->next_polynomial){
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
    else{
        if(!m_update_chunk){
            m_update_chunk = !m_update_chunk;
            fwrite(m_chunk->chunk_data, sizeof(char), m_chunk->length,myfile);// <--- JM DEBUG
            fclose(myfile);// <--- JM DEBUG
            return {m_chunk->chunk_data, m_chunk->length};
        }
        else{
            m_update_chunk = !m_update_chunk;
            m_chunk = m_chunk->next_polynomial;
            return next_chunk() ;
        }
    }
}

// ---------------------------------------------------------------------

} // namespace uh::client::chunking
