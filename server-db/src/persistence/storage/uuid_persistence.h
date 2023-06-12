//
// Created by max on 01.06.23.
//

#ifndef CORE_UUID_PERSISTENCE_H
#define CORE_UUID_PERSISTENCE_H

#include <filesystem>
#include <options/options.h>
#include <persistence/options.h>

namespace uh::dbn::persistence
{
// ---------------------------------------------------------------------

    /*
     * Class to store the uuid uniquely identifying the data node without relying on network addresses or host names.
     */
    class uuid_persistence
    {
    public:
        explicit uuid_persistence(const uh::options::persistence_config& config);
        uuid_persistence();

        void start();
        std::string getIdentity();

    private:
        void retrieve();
        void initialize();
        std::filesystem::path m_target_path;
        std::string m_uuid;
    };

// ---------------------------------------------------------------------
}


#endif //CORE_UUID_PERSISTENCE_H
