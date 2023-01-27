#ifndef SERIALIZATION_RECOMPILATION_H
#define SERIALIZATION_RECOMPILATION_H

#include <utility>
#include "Data.h"
#include "../client_options/client_config.h"
#include "recomp_tree.h"
#include <protocol/client.h>

namespace uh::client::serialization
{

// ---------------------------------------------------------------------

class Recompilation final : virtual public std::fstream {
    public:
        Recompilation(const client_config& config, std::unique_ptr<uh::protocol::client>&& client);
        ~Recompilation() final;

    protected:
        void encode();
        void decode();
        void ls();

    private:
        const client_config& m_config;
        std::unique_ptr<uh::protocol::client> m_client;
        std::list<Data> data;
        std::shared_ptr<treeNode<std::string,std::uint64_t>> rootShrPtr{};
};

// ---------------------------------------------------------------------

} // namespace uh::client::serialization

#endif
