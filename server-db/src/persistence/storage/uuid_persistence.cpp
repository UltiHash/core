//
// Created by max on 01.06.23.
//

#include "uuid_persistence.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <logging/logging_boost.h>
#include <io/temp_file.h>
#include <io/file.h>
#include <serialization/serialization.h>

namespace uh::dbn::persistence
{

// ---------------------------------------------------------------------

    uuid_persistence::uuid_persistence(const uh::options::persistence_config& config) :
            m_target_path(config.persistence_path / std::filesystem::path("uuid.uhs"))
    {
    }

// ---------------------------------------------------------------------

    uuid_persistence::uuid_persistence() :
            m_target_path("/tmp/uh/data-node" / std::filesystem::path("uuid.uhs"))
    {
    }

// ---------------------------------------------------------------------

    void uuid_persistence::start()
    {
        if (std::filesystem::exists(m_target_path))
        {
            retrieve();
        }
        else
        {
            initialize();
        }



        INFO << "data node uuid persisted at: " << m_target_path;
    }

// ---------------------------------------------------------------------

    void uuid_persistence::initialize()
    {
        io::temp_file uuid(m_target_path.parent_path());

        uh::serialization::buffered_serializer serializer(uuid);
        boost::uuids::random_generator rng;
        m_uuid = boost::uuids::to_string(rng());
        serializer.write(m_uuid);
        serializer.sync();
        uuid.rename(m_target_path);
    }

// ---------------------------------------------------------------------

    void uuid_persistence::retrieve()
    {
        io::file uuid(m_target_path);
        uh::serialization::sl_deserializer deserializer(uuid);

        m_uuid = deserializer.read<std::string>();
    }

// ---------------------------------------------------------------------

}