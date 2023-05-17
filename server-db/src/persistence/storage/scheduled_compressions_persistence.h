#ifndef SERVER_DATABASE_PERSISTENCE_SCHEDULED_COMPRESSIONS_PERSISTENCE_H
#define SERVER_DATABASE_PERSISTENCE_SCHEDULED_COMPRESSIONS_PERSISTENCE_H

#include <filesystem>
#include <set>
#include <options/options.h>
#include <persistence/options.h>

namespace uh::dbn::persistence
{

// ---------------------------------------------------------------------

    /*
     * Class to store the scheduling information of the compression in a device. It is not thread safe.
     * Other classes using it which are multithreaded should have thread safety built-in in order to access
     * and use the scheduled_compressions_persistence class.
     */
    class scheduled_compressions_persistence
    {
    public:
        explicit scheduled_compressions_persistence(const persistence_config& config);
        scheduled_compressions_persistence();

        void start();
        std::pair<std::set<std::filesystem::path>::iterator, bool> emplace(const std::filesystem::path& path);
        void erase(const std::filesystem::path& path);
        void flush();
        [[nodiscard]] std::size_t size() const;

    private:
        void retrieve();
        std::filesystem::path m_target_path;
        std::set<std::filesystem::path> m_scheduled{};
    };

// ---------------------------------------------------------------------

} // namespace uh::dbn::persistence

#endif
