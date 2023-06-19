#ifndef CHUNKING_MOD_H
#define CHUNKING_MOD_H

#include <chunking/defaults.h>
#include <util/exception.h>

#include <chunking/chunker.h>
#include <chunking/gear.h>
#include <chunking/fast_cdc.h>
#include <chunking/rabin_fp.h>

#include <unordered_map>
#include "chunking/mod_chunker.h"


namespace uh::chunking
{

enum class ChunkingStrategy
{
    FixedSize,
    CDCrabin,
    Gear,
    FastCDC,
    ModCDC
};

constexpr const char* strategyString(ChunkingStrategy n)
{
    switch (n)
    {
        case ChunkingStrategy::FixedSize: return "FixedSize";
        case ChunkingStrategy::CDCrabin: return "CDCrabin";
        case ChunkingStrategy::Gear: return "Gear";
        case ChunkingStrategy::FastCDC: return "FastCDC";
        case ChunkingStrategy::ModCDC: return "ModCDC";
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
  {strategyString(ChunkingStrategy::CDCrabin), ChunkingStrategy::CDCrabin},
  {strategyString(ChunkingStrategy::Gear), ChunkingStrategy::Gear},
  {strategyString(ChunkingStrategy::FastCDC), ChunkingStrategy::FastCDC},
  {strategyString(ChunkingStrategy::ModCDC), ChunkingStrategy::ModCDC},

};


// ---------------------------------------------------------------------

struct config
{
    constexpr static std::string_view default_chunking_strategy =
        strategyString(ChunkingStrategy::FixedSize);

    std::string chunking_strategy = std::string(default_chunking_strategy);

    constexpr static size_t default_chunk_size_in_bytes = DEFAULT_CHUNK_SIZE;
    size_t chunk_size_in_bytes = 0;

    uh::chunking::fast_cdc_config fast_cdc;
    uh::chunking::gear_config gear;
    uh::chunking::mod_cdc_config mod_cdc;
};

// ---------------------------------------------------------------------

class mod
{
public:
    explicit mod(const config& cfg);

    std::unique_ptr<uh::chunking::chunker> create_chunker(io::device& d, std::size_t file_size);

private:
    ChunkingStrategy m_strategy;
    size_t m_chunk_size;
    uh::chunking::fast_cdc_config m_fast_cdc;
    uh::chunking::gear_config m_gear;
    uh::chunking::mod_cdc_config m_mod_cdc;
};

// ---------------------------------------------------------------------

} // namespace uh::chunking

#endif
