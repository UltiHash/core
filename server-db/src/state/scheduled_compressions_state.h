#ifndef SERVER_DATABASE_STATE_SCHEDULED_COMPRESSION_STATE_H
#define SERVER_DATABASE_STATE_SCHEDULED_COMPRESSION_STATE_H

#include <filesystem>
#include <set>
#include <storage/storage_config.h>

namespace uh::dbn::state
{

// ---------------------------------------------------------------------

    /*
     * Class to store the scheduling information of the compression in a device. It is not thread safe.
     * Other classes using it which are multi-threaded should have thread safety built-in in order to access
     * and use the scheduled_compressions_state class.
     */
    class scheduled_compressions_state
    {
    public:
        explicit scheduled_compressions_state(const uh::dbn::storage::storage_config& config);
        scheduled_compressions_state();

        void start();
        std::pair<std::set<std::filesystem::path>::iterator, bool> insert(const std::filesystem::path& path);
        void erase(const std::filesystem::path& path);
        void flush();

        [[nodiscard]] const std::set<std::filesystem::path>& set() const;
        [[nodiscard]] bool empty() const;
        [[nodiscard]] std::size_t size() const;

    private:
        void retrieve();
        std::filesystem::path m_target_path;
        std::set<std::filesystem::path> m_scheduled{};
    };

// ---------------------------------------------------------------------

} // namespace uh::dbn::state

#endif
