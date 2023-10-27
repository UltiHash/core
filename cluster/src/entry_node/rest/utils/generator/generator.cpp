#include "generator.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace uh::cluster::rest::utils::generator
{

    std::string generate_unique_id()
    {
        boost::uuids::random_generator gen;
        boost::uuids::uuid uuid = gen();

        return boost::uuids::to_string(uuid);
    }

} // uh::cluster::rest::utils::generator
