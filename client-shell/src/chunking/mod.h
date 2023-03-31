#ifndef CLIENT_CHUNKING_MOD_H
#define CLIENT_CHUNKING_MOD_H
#include <protocol/client_pool.h>

#include <chunking/file_chunker.h>


namespace uh::client::chunking
{

enum class ChunkingStrategyEnum {FixedSize, OtherChunkingStrategy};

/*
  Chunking can be done by following one of several strategies: For instance, Fixed size chunking
  splits a file in several equal-sized chunks. For such a strategy, different chunk sizes can be
  chosen. Another strategy is Content Defined Chunking (CDC), for which several algorithms exist.
  A minimum chunk size and a maximum chunk size can be defined as well.
*/

static std::unordered_map<std::string, ChunkingStrategyEnum> string2backendtype =
{
  {"FixedSize", ChunkingStrategyEnum::FixedSize},
  {"OtherChunkingStrategy", ChunkingStrategyEnum::OtherChunkingStrategy}
};

// ---------------------------------------------------------------------

struct chunking_config
{
    constexpr static std::string_view default_chunking_strategy = "FixedSize";
    std::string chunking_strategy = std::string(default_chunking_strategy);

    constexpr static size_t default_chunk_size_in_bytes = 4194304; // = 2^22
    size_t chunk_size_in_bytes = 0;
};

// ---------------------------------------------------------------------

class mod
{
public:
    explicit mod(const chunking_config& cfg);
    ~mod();

    chunking::file_chunker& start();

    chunking::file_chunker& chunker();

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::client::chunking

#endif
