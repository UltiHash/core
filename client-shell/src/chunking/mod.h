#ifndef CLIENT_CHUNKING_MOD_H
#define CLIENT_CHUNKING_MOD_H

#include <protocol/client_pool.h>

#include <chunking/chunker.h>
#include <util/exception.h>


namespace uh::client::chunking
{

enum class ChunkingStrategyEnum {FixedSize, OtherChunkingStrategy};

constexpr const char* strategyString(ChunkingStrategyEnum n)
{
    switch (n)
    {
        case ChunkingStrategyEnum::FixedSize: return "FixedSize";
        case ChunkingStrategyEnum::OtherChunkingStrategy: return "OtherChunkingStrategy";
        default: THROW(util::exception, "Not implemented option");
    }
}
/*
  Chunking can be done by following one of several strategies: For instance, Fixed size chunking
  splits a file in several equal-sized chunks. For such a strategy, different chunk sizes can be
  chosen. Another strategy is Content Defined Chunking (CDC), for which several algorithms exist.
  A minimum chunk size and a maximum chunk size can be defined as well.
*/

static std::unordered_map<std::string, ChunkingStrategyEnum> string2backendtype =
{
  {strategyString(ChunkingStrategyEnum::FixedSize), ChunkingStrategyEnum::FixedSize},
  {strategyString(ChunkingStrategyEnum::OtherChunkingStrategy), ChunkingStrategyEnum::OtherChunkingStrategy},
};


// ---------------------------------------------------------------------

struct chunking_config
{
    constexpr static std::string_view default_chunking_strategy = strategyString(ChunkingStrategyEnum::FixedSize);
    std::string chunking_strategy = std::string(default_chunking_strategy);

    constexpr static size_t default_chunk_size_in_bytes = 4 * 1024 * 1024;
    size_t chunk_size_in_bytes = 0;
};

// ---------------------------------------------------------------------

class mod
{
public:
    explicit mod(const chunking_config& cfg);

    std::unique_ptr<uh::chunking::chunker> create_chunker(io::device& d);

private:
    ChunkingStrategyEnum m_strategy;
    size_t m_chunk_size;
};

// ---------------------------------------------------------------------

} // namespace uh::client::chunking

#endif
