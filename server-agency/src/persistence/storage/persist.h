#ifndef SERVER_AGENCY_PERSISTENCE_PERSIST_H
#define SERVER_AGENCY_PERSISTENCE_PERSIST_H

#include "protocol/messages.h"

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

    class persist
    {
    public:

        virtual ~persist() = default;

        virtual void start() = 0;

        virtual void add(const uh::protocol::client_statistics::request& req) = 0;

        virtual void retrieve() = 0;

        virtual void flush() = 0;

    };

// ---------------------------------------------------------------------

} // namespace uh::an::persistence

#endif
