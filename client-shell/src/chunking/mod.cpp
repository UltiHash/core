#include "mod.h"

#include <logging/logging_boost.h>
#include "chunking/fixed_size_chunker.h"
#include <chunking/mod_chunker.h>


namespace uh::client::chunking
{

namespace
{

// ---------------------------------------------------------------------

ChunkingStrategy define_chunking_strategy(const std::string& chunking_strategy)
{
    auto it = string2backendtype.find(chunking_strategy);
    if (it != string2backendtype.end()) {
        return it->second;
    } else {
        std::string msg("Not a chunking strategy: " + chunking_strategy);
        ERROR << msg;
        THROW(util::exception, msg);
    }
}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

mod::mod(const chunking_config& cfg)
    : m_strategy(define_chunking_strategy(cfg.chunking_strategy)),
      m_chunk_size(cfg.chunk_size_in_bytes),
      m_fast_cdc(cfg.fast_cdc),
      m_gear(cfg.gear),
      m_mod_cdc(cfg.mod_cdc),
      m_rabin(cfg.rabin)
{
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::chunking::chunker> mod::create_chunker(io::device& d, std::size_t buffer_size)
{
    switch (m_strategy)
    {
        case ChunkingStrategy::FixedSize:
            return std::make_unique<uh::chunking::fixed_size_chunker>(d, m_chunk_size, buffer_size);
        case ChunkingStrategy::Gear:
            return std::make_unique<uh::chunking::gear>(m_gear, d, buffer_size);
        case ChunkingStrategy::FastCDC:
            return std::make_unique<uh::chunking::fast_cdc>(m_fast_cdc, d, buffer_size);
        case ChunkingStrategy::ModCDC:
            return std::make_unique<uh::chunking::mod_chunker>(m_mod_cdc, d);
        case ChunkingStrategy::CDCrabin:
            return std::make_unique<uh::chunking::rabin_fp>(m_rabin, d);
    }

    THROW(util::exception, "chunk type not implemented");
}

// ---------------------------------------------------------------------

} //namespace uh::client::chunking
