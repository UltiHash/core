#include <mutex>
#include <uhv/f_meta_data.h>

namespace uh::uhv {

// ---------------------------------------------------------------------

    class ts_container {
    public:

        class container_handle
        {

        public:
            ~container_handle();

            std::unordered_map <std::string, uh::uhv::f_meta_data>& operator()();

        private:
            friend ts_container;

            container_handle(ts_container& container, std::unordered_map <std::string, uh::uhv::f_meta_data>& paths);

            ts_container &m_container;
            std::unordered_map <std::string, uh::uhv::f_meta_data>& m_paths_metadata;

        };

        ts_container() = default;
        container_handle get();
        std::unordered_map <std::string, uh::uhv::f_meta_data>& n_ts_get();

    private:
        friend class container_handle;

        void unlock();

        std::mutex m_mutex;
        std::unordered_map <std::string, uh::uhv::f_meta_data> m_paths_metadata;
    };

// ---------------------------------------------------------------------

} // namespace uh::uhv
