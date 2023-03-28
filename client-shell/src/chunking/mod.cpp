#include "mod.h"

#include <config.hpp>

#include <logging/logging_boost.h>
#include <util/exception.h>
#include <chunking/strategies/fixed_size_chunker.h>


namespace uh::client::chunking
{

namespace
{

// ---------------------------------------------------------------------

ChunkingStrategyEnum define_chunking_strategy(std::string chunking_strategy){
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

std::unique_ptr<file_chunker> make_chunker(const chunking_config& cfg){

    //Check whether we have enough space:
    auto chunking_strategy = define_chunking_strategy(cfg.chunking_strategy);

    switch (chunking_strategy)
    {
        case ChunkingStrategyEnum::FixedSize:
            return std::make_unique<chunking::fixed_size_chunker>(cfg.chunk_size_in_bytes);
        case ChunkingStrategyEnum::OtherChunkingStrategy:
            THROW(util::exception, "Not implemented yet");
    }

    std::string msg("Not a chunking strategy: " + cfg.chunking_strategy);
    THROW(util::exception, msg);

}

// ---------------------------------------------------------------------

} // namespace

// ---------------------------------------------------------------------

struct mod::impl
{
    impl(const chunking_config& cfg);
    std::unique_ptr<chunking::file_chunker> m_chunker;
};

// ---------------------------------------------------------------------

mod::impl::impl(const chunking_config& cfg)
    : m_chunker(make_chunker(cfg))
{
}

// ---------------------------------------------------------------------

mod::mod(const chunking_config& cfg)
    : m_impl(std::make_unique<impl>(cfg))
{
}

// ---------------------------------------------------------------------

mod::~mod() = default;

// ---------------------------------------------------------------------

void mod::start()
{
    INFO << "starting file chunking module";
    m_impl->m_chunker->start();
}

// ---------------------------------------------------------------------

chunking::file_chunker& mod::chunker()
{
    return *m_impl->m_chunker;
}

// ---------------------------------------------------------------------

} //namespace uh::client::chunking