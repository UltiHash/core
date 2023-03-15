#include "options.h"


using namespace boost::program_options;

namespace uh::client::chunking
{

enum class OptionsEnum {ChunkingStrategy, ChunkSize};

constexpr const char* optionString(OptionsEnum n)
{
    switch (n)
    {
        case OptionsEnum::ChunkingStrategy: return "chunking-strategy";
        case OptionsEnum::ChunkSize: return "chunk-size";
        default: THROW(util::exception, "Not implemented option");
    }
}




// ---------------------------------------------------------------------

options::options()
    : uh::options::options("Storage Options")
{
    visible().add_options()
        (optionString(OptionsEnum::ChunkingStrategy),
            value<std::string>()->default_value(std::string(uh::client::chunking::chunking_config::default_chunking_strategy)),
                "Strategy to use for spliting files into chunks")
        (optionString(OptionsEnum::ChunkSize),
            value<size_t>()->default_value(uh::client::chunking::chunking_config::default_chunk_size_in_bytes),
                "Size in bytes of the chunks, in case of a fixed size chunking strategy");
}

// ---------------------------------------------------------------------

uh::options::action options::evaluate(const boost::program_options::variables_map& vars)
{
    chunking_config c;
    c.chunking_strategy = vars[optionString(OptionsEnum::ChunkingStrategy)].as<std::string>();

    size_t chunk_size = vars[optionString(OptionsEnum::ChunkSize)].as<std::size_t>();
    c.chunk_size_in_bytes = chunk_size;


    std::swap(m_config, c);
    return uh::options::action::proceed;
}

// ---------------------------------------------------------------------

const chunking_config& options::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

} // namespace boost::program_options;