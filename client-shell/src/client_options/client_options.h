#ifndef CLIENT_OPTIONS_CLIENT_OPTIONS_H
#define CLIENT_OPTIONS_CLIENT_OPTIONS_H

#include <iostream>
#include <filesystem>
#include <list>
#include <options/options.h>
#include "client_config.h"
#include "agency_connection.h"

namespace uh::client
{

// ---------------------------------------------------------------------

class client_options : public uh::options::options
    {
    public:

        // CLASS FUNCTIONS
        client_options();

        virtual uh::options::action evaluate(const boost::program_options::variables_map& vars) override;

        // GETTERS
        [[nodiscard]] bool optDisabled() const;

        // LOGIC FUNCTIONS
        void handle(const boost::program_options::variables_map& vars, client_config& config) const;
        void conflictingOptions() const;

        const client_config& config() const;

    private:
        bool m_retrieve = false;
        bool m_integrate = false;
        bool m_list = false;
        bool m_exclude = false;

    private:
        client_config m_config;
        std::vector<std::string> m_posPaths;
        std::vector<std::string> m_operateStrPaths;
        std::string m_targetDirectory;
    };

// ---------------------------------------------------------------------

} // namespace uh::client

#endif

