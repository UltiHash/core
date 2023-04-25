#include "mod.h"

#include <logging/logging_boost.h>
#include <chunking/strategies/fixed_size_chunker.h>
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
      m_mod_cdc(cfg.mod_cdc)

{
}

// ---------------------------------------------------------------------

std::unique_ptr<uh::chunking::chunker> mod::create_chunker(io::device& d)
{
    switch (m_strategy)
    {
        case ChunkingStrategy::FixedSize:
            return std::make_unique<fixed_size_chunker>(d, m_chunk_size);
        case ChunkingStrategy::Mod:
            return std::make_unique<uh::chunking::mod_chunker>(m_mod_cdc, d);
        case ChunkingStrategy::Gear:
            return std::make_unique<uh::chunking::gear>(m_gear, d);
        case ChunkingStrategy::FastCDC:
            return std::make_unique<uh::chunking::fast_cdc>(m_fast_cdc, d);
    }

    THROW(util::exception, "chunk type not implemented");
}

// ---------------------------------------------------------------------

} //namespace uh::client::chunking
