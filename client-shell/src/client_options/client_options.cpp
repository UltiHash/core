#include "client_options.h"
#include <logging/logging_boost.h>
#include <unordered_set>

using namespace boost::program_options;

namespace uh::client::option
{

// ---------------------------------------------------------------------

client_options::client_options()
        : options("Client Options")
{
    visible().add_options()
            ("retrieve,r", value<std::string>(&m_uhv_path), "read the UltiHash Volume and put the contents to the target destination")
            ("integrate,i", value<std::string>(&m_uhv_path), "write the contents of the sources provided and generate a UltiHash Volume file at the target")
            ("jobs,j",  value<std::uint16_t>(&m_config.m_worker_count), "size of the worker threads when uploading and downloading")
            ("exclude,E", value<std::vector<std::string>>(&m_operateStrPaths)->multitoken(), "exclude directories when integrating [optional]")
            ("target,T", value<std::string>(&m_targetDirectory), "destination of the target directory for --retrieve(-r) operation [optional]")
            ("verbose,V" , "shows details about the results of running the command [optional]");
    hidden().add_options()
            ("positional,p", value<std::vector<std::string>>(&m_posPaths)->multitoken(),"[default] positional arguments given");

    positional_mapping("positional", -1);
}

// ---------------------------------------------------------------------

bool client_options::optDisabled() const
{
    return !(m_integrate || m_retrieve || m_list);
}

// ---------------------------------------------------------------------

void option_dependency(const boost::program_options::variables_map & vm,
                       const std::string & for_what, const std::string & required_option)
{
    if (vm.count(for_what) && !vm[for_what].defaulted())
        if (vm.count(required_option) == 0 || vm[required_option].defaulted())
            throw std::logic_error(std::string("Option '") + for_what
                                   + "' requires option '" + required_option + "'. Please refer to --help for more information.");
}

// ---------------------------------------------------------------------

uh::options::action client_options::evaluate(const boost::program_options::variables_map& vars)
{
    m_retrieve = vars.count("retrieve") != 0;
    m_integrate = vars.count("integrate") != 0;
    m_list = vars.count("list") != 0;
    m_exclude = vars.count("exclude") != 0;

    if (vars.count("target") != 0)
        option_dependency(vars,"target", "retrieve");
    if (vars.count("exclude") != 0)
        option_dependency(vars,"exclude", "integrate");

    conflictingOptions();
    handle(vars);
    return uh::options::action::proceed;
}

// ---------------------------------------------------------------------

void client_options::conflictingOptions() const
{
    if ((m_retrieve && m_integrate) || (m_integrate && m_list) || (m_retrieve && m_list))
    {
        throw std::logic_error("Multiple conflicting client options chosen. See --help for more information.");
    }
}

// ---------------------------------------------------------------------

const client_config& client_options::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

void client_options::handle(const boost::program_options::variables_map& vars)
{
    if (optDisabled())
        throw std::invalid_argument("No client options given. See --help for more information.");

    std::string errorPath;

    //------------------------------------------------ LAMDA FUNCTIONS

    //lamda function for checking UltiHash Volume
    auto is_UHV = [](const std::vector<std::filesystem::path>& input, const std::string& chosenOpt)
    {
        for (const auto& m_path: input)
        {
            if (m_path.extension()!=".uh")
                throw std::logic_error(chosenOpt);
        }
    };
    //------------------------------------------------ END OF LAMDA FUNCTIONS

    //------------------------------------------------ SANITY CHECKS ACCORDING TO OPTION

    // generation of filesystem paths from CLI strings
    if (m_integrate)
    {
        m_config.m_outputPath  = weakly_canonical(std::filesystem::path(m_uhv_path));
        is_UHV({m_config.m_outputPath}, "destination on --integrate[-i] has wrong extensions. Please ensure that the destination ends with '.uh'.");

        if (exists(m_config.m_outputPath))
        {
            std::string user_response;
            std::unordered_set<std::string> validResponses = {"y", "n", "yes", "no"};

            std::cout << "The file already exists. Do you want to overwite it? (y/n) ";
            std::cin >> user_response;

            std::ranges::transform(user_response, user_response.begin(), [](char c){ return std::tolower(c); });

            if (validResponses.count(user_response) > 0)
            {
                if (user_response == "y" || user_response == "yes")
                {
                    m_config.m_overwrite = true;
                }
                else
                {
                    throw std::logic_error("Please provide a new UHV file.");
                }
            }
            else
            {
                throw std::runtime_error("Invalid response. Please enter 'y' or 'n'.");
            }
        }

        if (m_posPaths.empty())
            throw std::runtime_error("No data specified to be integrated. Please refer to --help more for information.");
        if (m_posPaths.size() > 1)
            throw std::runtime_error( "Too many arguments for the integrate operation.");
        if (m_exclude)
            for (const auto& path : m_operateStrPaths)
                m_config.m_operatePaths.emplace_back(canonical(std::filesystem::path(path)));
        try
        {
            std::set<std::filesystem::path> removeDuplicatePath;
            for (const auto & m_posPath : m_posPaths)
            {
                errorPath = m_posPath;
                auto sanitizedPath = std::filesystem::canonical(
                        std::filesystem::path(errorPath));
                removeDuplicatePath.insert(sanitizedPath);
            }
            m_config.m_inputPaths.assign(removeDuplicatePath.begin(), removeDuplicatePath.end());
        }
        catch(const std::exception& ex)
        {
            throw std::runtime_error( "'" + errorPath + "' doesn't exists.");
        }
        m_config.m_option = options_chosen::integrate;
    }
    else if (m_retrieve)
    {
        if (vars.contains("target"))
        {
            auto targetDirectory = weakly_canonical(std::filesystem::path(m_targetDirectory));

            if (!std::filesystem::exists(targetDirectory))
                throw std::runtime_error("--target(-T) :" + targetDirectory.string() + " doesn't exists.");

            if (!is_directory(targetDirectory))
                throw std::runtime_error("--target(-T) requires a directory path.");

            m_config.m_outputPath = targetDirectory;
        }
        else
        {
            m_config.m_outputPath = std::filesystem::current_path().string()+"/UHOutput";
            std::filesystem::create_directory(m_config.m_outputPath);
        }
        try
        {
            errorPath = m_uhv_path;
            m_config.m_inputPaths.push_back(canonical(std::filesystem::path(errorPath)));
            is_UHV(m_config.m_inputPaths, "source path on --retrieve[-r] has wrong extensions. Please ensure that the source ends with '.uh'.");
        }
        catch(const std::exception& ex)
        {
            throw std::runtime_error( "'" + errorPath + "' doesn't exists.");
        }
        if (!m_posPaths.empty())
        {
            throw std::runtime_error( "Too many arguments for the retrieve operation.");
        }
        m_config.m_option = options_chosen::retrieve;
    }
    else if (m_list)
    {
        try {
            errorPath = m_posPaths.at(0);
            m_config.m_inputPaths.push_back(canonical(std::filesystem::path(errorPath)));
            is_UHV(m_config.m_inputPaths, "source path on --list[-l] has wrong extensions. Please ensure that the source ends with '.uh'.");

            if (m_posPaths.size()>1)
            {
                for (auto i = 1u; i < m_posPaths.size(); ++i) {
                    errorPath = m_posPaths.at(i);
                    m_config.m_operatePaths.emplace_back(weakly_canonical(std::filesystem::path(errorPath)));
                }
            } else
            {
                m_config.m_operatePaths.emplace_back("/");
            }
        }
        catch(const std::exception& ex)
        {
            throw std::runtime_error( "'" + errorPath + "' doesn't exists.");
        }
        m_config.m_option = options_chosen::list;
    }
    //------------------------------------------------ END OF BASIC SANITY CHECKS

    // checking inputs and outputs for validation
    std::cout << "INPUT: ";
    for (const auto& path: m_config.m_inputPaths)
    {
        std::cout << path << " ";
    }
    std::cout << "\nOUTPUT: ";
    std::cout << m_config.m_outputPath << "\n";
}

// ---------------------------------------------------------------------

} // namespace uh::client::option
