#ifndef CLIENT_OPTIONS_CLIENT_OPTIONS_H
#define CLIENT_OPTIONS_CLIENT_OPTIONS_H

#include <iostream>
#include <filesystem>
#include <list>
#include <options/options.h>
#include "client_config.h"
#include "agency_connection.h"

namespace uh::client::option
{

// ---------------------------------------------------------------------

class client_options
    {
    public:

        // CLASS FUNCTIONS
        client_options();

        // GETTERS
        [[nodiscard]] bool optDisabled() const;

        // LOGIC FUNCTIONS
        void apply(uh::options::options& opts);
        void evaluate(const boost::program_options::variables_map& vars);
        void handle(const boost::program_options::variables_map& vars, client_config& config) const;
        void conflictingOptions() const;

    private:
        bool m_retrieve = false;
        bool m_integrate = false;
        bool m_list = false;
        bool m_exclude = false;

    private:
        boost::program_options::options_description m_desc;
        boost::program_options::options_description m_hiddenDesc;
        std::vector<std::string> m_posPaths;
        std::vector<std::string> m_operateStrPaths;
        std::string m_targetDirectory;
    };

// ---------------------------------------------------------------------

} // namespace uh::client

#endif

