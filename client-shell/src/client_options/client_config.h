#ifndef CLIENT_OPTIONS_CLIENT_CONFIG_H
#define CLIENT_OPTIONS_CLIENT_CONFIG_H

#include <filesystem>
#include <vector>

namespace uh::client::option
{

// ---------------------------------------------------------------------

enum class options_chosen : char
{
    retrieve = 'r',
    integrate = 'i',
    list = 'l'
};

// ---------------------------------------------------------------------

typedef struct
{
    std::uint16_t m_worker_count;
    options_chosen m_option;
    std::vector<std::filesystem::path> m_inputPaths;
    std::filesystem::path m_outputPath;
    std::vector<std::filesystem::path> m_operatePaths;
}  client_config;


// ---------------------------------------------------------------------

} // namespace uh::client

#endif

