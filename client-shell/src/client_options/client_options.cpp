#include "client_options.h"
#include <logging/logging_boost.h>

using namespace boost::program_options;

namespace uh::client::option
{

// ---------------------------------------------------------------------

client_options::client_options()
        : options("Client Options")
{
    visible().add_options()
            ("retrieve,r","read the recompilation file and put the contents to the target destination")
            ("integrate,i","write the contents of the sources provided and generate the recompilation file at the target")
            ("workers,w",  value<std::uint16_t>(&m_config.m_worker_count), "size of the worker threads when uploading and downloading")
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
    option_dependency(vars,"target", "retrieve");
    option_dependency(vars,"exclude", "integrate");
    m_retrieve = vars.count("retrieve") != 0;
    m_integrate = vars.count("integrate") != 0;
    m_list = vars.count("list") != 0;
    m_exclude = vars.count("exclude") != 0;
    conflictingOptions();
    handle(vars, m_config);
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

void client_options::handle(const boost::program_options::variables_map& vars, client_config& config) const
{
    if (optDisabled())
        throw std::invalid_argument("No client options given. See --help for more information.");

    // evaluate the commands, check if files are relevant and put it in a data structure
    std::vector<std::filesystem::path> destPaths;
    std::string errorPath;

    //------------------------------------------------ LAMDA FUNCTIONS
    // lamda function for checking if path exists
    auto is_Existing = [](std::vector<std::filesystem::path>& input)
    {
        for (const auto& Path : input)
        {
            if (std::filesystem::exists(Path))
                throw std::runtime_error("'"+Path.string() + "' already exists.");
        }
    };

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
    if (m_posPaths.empty())
        throw std::runtime_error("Path missing for the given client option.");

    // generation of filesystem paths from CLI strings
    if (m_integrate)
    {
        destPaths.push_back(weakly_canonical(std::filesystem::path(m_posPaths.front())));
        is_UHV(destPaths, "destination on --integrate[-i] has wrong extensions. Please ensure that the destination ends with '.uh'.");
        is_Existing(destPaths);
        if (m_posPaths.size()==1)
            throw std::runtime_error("--integrate[-i] requires a source and a target. Please refer to --help more for information.");
        if (m_exclude)
            for (const auto& path : m_operateStrPaths)
                config.m_operatePaths.emplace_back(canonical(std::filesystem::path(path)));
        try
        {
            std::set<std::filesystem::path> removeDuplicatePath;
            for (int i = 1; i < m_posPaths.size(); ++i)
            {
                errorPath = m_posPaths.at(i);
                auto sanitizedPath = std::filesystem::canonical(
                        std::filesystem::path(errorPath));
                removeDuplicatePath.insert(sanitizedPath);
            }
            config.m_inputPaths.assign(removeDuplicatePath.begin(), removeDuplicatePath.end());
        }
        catch(const std::exception& ex)
        {
            throw std::runtime_error( "'" + errorPath + "' doesn't exists.");
        }
        config.m_option = options_chosen::integrate;
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

            destPaths.push_back(targetDirectory);
        }
        else
        {
            destPaths.emplace_back(std::filesystem::current_path().string()+"/UHOutput");
            std::filesystem::create_directory(destPaths[0]);
        }
        try
        {
            errorPath = m_posPaths.at(0);
            config.m_inputPaths.push_back(canonical(std::filesystem::path(errorPath)));
            is_UHV(config.m_inputPaths, "source path on --retrieve[-r] has wrong extensions. Please ensure that the source ends with '.uh'.");
            if (m_posPaths.size()>1)
            {
                for (int i = 1; i < m_posPaths.size(); ++i)
                {
                    errorPath = m_posPaths.at(i);
                    config.m_operatePaths.emplace_back(weakly_canonical(std::filesystem::path(errorPath)));
                }
            }
        }
        catch(const std::exception& ex)
        {
            throw std::runtime_error( "'" + errorPath + "' doesn't exists.");
        }
        config.m_option = options_chosen::retrieve;
    }
    else if (m_list)
    {
        try {
            errorPath = m_posPaths.at(0);
            config.m_inputPaths.push_back(canonical(std::filesystem::path(errorPath)));
            is_UHV(config.m_inputPaths, "source path on --list[-l] has wrong extensions. Please ensure that the source ends with '.uh'.");

            if (m_posPaths.size()>1)
            {
                for (int i = 1; i < m_posPaths.size(); ++i) {
                    errorPath = m_posPaths.at(i);
                    config.m_operatePaths.emplace_back(weakly_canonical(std::filesystem::path(errorPath)));
                }
            } else
            {
                config.m_operatePaths.emplace_back("/");
            }
        }
        catch(const std::exception& ex)
        {
            throw std::runtime_error( "'" + errorPath + "' doesn't exists.");
        }
        config.m_option = options_chosen::list;
    }
    //------------------------------------------------ END OF BASIC SANITY CHECKS

    if (!m_list)
        config.m_outputPath = destPaths.at(0);

    // checking inputs and outputs for validation
    std::cout << "INPUT: ";
    for (const auto& path: config.m_inputPaths)
    {
        std::cout << path << " ";
    }
    std::cout << "\nOUTPUT: ";
    std::cout << config.m_outputPath << "\n";
}

// ---------------------------------------------------------------------

} // namespace uh::client::option