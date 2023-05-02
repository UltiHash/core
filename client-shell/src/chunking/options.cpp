#include "options.h"

#include <util/exception.h>


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
    : uh::options::options("Chunking Options")
{
    visible().add_options()
        (optionString(OptionsEnum::ChunkingStrategy),
            value<std::string>()->default_value(std::string(uh::client::chunking::chunking_config::default_chunking_strategy)),
                "Strategy to use for spliting files into chunks")
        (optionString(OptionsEnum::ChunkSize),
            value<size_t>()->default_value(uh::client::chunking::chunking_config::default_chunk_size_in_bytes),
                "Size in bytes of the chunks, in case of a fixed size chunking strategy")
        ("gear-max-size", value< std::size_t >(&m_config.gear.max_size), "maximum chunk size for Gear CDC")
        ("gear-avg-size", value< std::size_t >(&m_config.gear.average_size), "average chunk size for Gear CDC")
        ("fastcdc-min-size", value< std::size_t >(&m_config.fast_cdc.min_size), "minimum chunk size for FastCDC")
        ("fastcdc-max-size", value< std::size_t >(&m_config.fast_cdc.max_size), "maximum chunk size for FastCDC")
        ("fastcdc-normal-size", value< std::size_t >(&m_config.fast_cdc.normal_size), "normal chunk size for FastCDC")
        ("modcdc-min-size", value< std::size_t >(&m_config.mod_cdc.min_size), "minimum chunk size for ModCDC")
        ("modcdc-max-size", value< std::size_t >(&m_config.mod_cdc.max_size), "maximum chunk size for ModCDC")
        ("modcdc-normal-size", value< std::size_t >(&m_config.mod_cdc.normal_size), "normal chunk size for ModCDC");
}

// ---------------------------------------------------------------------

uh::options::action options::evaluate(const boost::program_options::variables_map& vars)
{
    chunking_config c = m_config;
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
