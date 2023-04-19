#include "mod.h"

#include <logging/logging_boost.h>
#include <util/exception.h>
#include <chunking/strategies/fixed_size_chunker.h>


namespace uh::client::chunking
{

namespace
{

// ---------------------------------------------------------------------

ChunkingStrategyEnum define_chunking_strategy(const std::string& chunking_strategy)
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
      m_chunk_size(cfg.chunk_size_in_bytes)
{
}

// ---------------------------------------------------------------------

std::unique_ptr<chunking::file_chunker> mod::create_chunker(io::device& d)
{
    switch (m_strategy)
    {
        case ChunkingStrategyEnum::FixedSize:
            return std::make_unique<fixed_size_chunker>(d, m_chunk_size);
    }

    THROW(util::exception, "chunk type not implemented");
}

// ---------------------------------------------------------------------

} //namespace uh::client::chunking
