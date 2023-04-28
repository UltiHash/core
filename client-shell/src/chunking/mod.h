#ifndef CLIENT_CHUNKING_MOD_H
#define CLIENT_CHUNKING_MOD_H

#include <chunking/defaults.h>
#include <util/exception.h>

#include <chunking/chunker.h>
#include <chunking/gear.h>
#include <chunking/fast_cdc.h>
#include <chunking/rabin_fingerprints_chunker.h>

#include <unordered_map>
#include "chunking/mod_chunker.h"


namespace uh::client::chunking
{

enum class ChunkingStrategy
{
    FixedSize,
    Mod,
    CDCrabin,
    Gear,
    FastCDC
};

constexpr const char* strategyString(ChunkingStrategy n)
{
    switch (n)
    {
        case ChunkingStrategy::FixedSize: return "FixedSize";
        case ChunkingStrategy::Mod: return "Mod";
        case ChunkingStrategy::CDCrabin: return "CDCrabin";
        case ChunkingStrategy::Gear: return "Gear";
        case ChunkingStrategy::FastCDC: return "FastCDC";
    }

    THROW(util::exception, "Not implemented option");
}


/*
  Chunking can be done by following one of several strategies: For instance, Fixed size chunking
  splits a file in several equal-sized chunks. For such a strategy, different chunk sizes can be
  chosen. Another strategy is Content Defined Chunking (CDC), for which several algorithms exist.
  A minimum chunk size and a maximum chunk size can be defined as well.
*/

static std::unordered_map<std::string, ChunkingStrategy> string2backendtype =
{
  {strategyString(ChunkingStrategy::FixedSize), ChunkingStrategy::FixedSize},
  {strategyString(ChunkingStrategy::Mod), ChunkingStrategy::Mod},
  {strategyString(ChunkingStrategy::CDCrabin), ChunkingStrategy::CDCrabin},
  {strategyString(ChunkingStrategy::Gear), ChunkingStrategy::Gear},
  {strategyString(ChunkingStrategy::FastCDC), ChunkingStrategy::FastCDC},
};


// ---------------------------------------------------------------------

struct chunking_config
{
    constexpr static std::string_view default_chunking_strategy =
        strategyString(ChunkingStrategy::FixedSize);

    std::string chunking_strategy = std::string(default_chunking_strategy);

    constexpr static size_t default_chunk_size_in_bytes = DEFAULT_CHUNK_SIZE;
    size_t chunk_size_in_bytes = 0;

    uh::chunking::fast_cdc_config fast_cdc;
    uh::chunking::gear_config gear;
    uh::chunking::mod_cdc_config mod_cdc;
    uh::chunking::rabin_fingerprints_config rabin;
};

// ---------------------------------------------------------------------

class mod
{
public:
    explicit mod(const chunking_config& cfg);

    std::unique_ptr<uh::chunking::chunker> create_chunker(io::device& d);

private:
    ChunkingStrategy m_strategy;
    size_t m_chunk_size;
    uh::chunking::fast_cdc_config m_fast_cdc;
    uh::chunking::gear_config m_gear;
    uh::chunking::mod_cdc_config m_mod_cdc;
    uh::chunking::rabin_fingerprints_config m_rabin;
};

// ---------------------------------------------------------------------

} // namespace uh::client::chunking

#endif
