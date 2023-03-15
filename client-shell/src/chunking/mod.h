#ifndef CLIENT_CHUNKING_MOD_H
#define CLIENT_CHUNKING_MOD_H
#include <protocol/client_pool.h>

#include <chunking/file_chunker.h>


namespace uh::client::chunking
{
enum class ChunkingStrategyEnum {FixedSize, OtherChunkingStrategy};

static std::unordered_map<std::string, ChunkingStrategyEnum> string2backendtype = {
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
    mod(const chunking_config& cfg);
    ~mod();

    void start();

    chunking::file_chunker& chunker();

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::client::chunking

#endif
