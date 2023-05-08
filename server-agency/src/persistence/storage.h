#ifndef SERVER_AGENCY_PERSISTENCE_PERSISTENT_STORAGE_H
#define SERVER_AGENCY_PERSISTENCE_PERSISTENT_STORAGE_H


namespace uh::an::persistence
{

// ---------------------------------------------------------------------

    class storage
    {
    public:

        virtual ~storage() = default;

        virtual void start() = 0;

    };

// ---------------------------------------------------------------------

} // namespace uh::an::persistence

#endif
