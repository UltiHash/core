#include <mutex>
#include <uhv/f_meta_data.h>

namespace uh::uhv {

// ---------------------------------------------------------------------

    class ts_f_meta_data {
    public:

        class meta_data_handle
        {

        public:
            ~meta_data_handle();

            uh::uhv::f_meta_data &operator()();

        private:
            friend ts_f_meta_data;

            meta_data_handle(ts_f_meta_data &ts_fmeta_class, uh::uhv::f_meta_data &file_meta);

            ts_f_meta_data &m_ts_fmeta_class;
            uh::uhv::f_meta_data &m_file_meta;

        };

        explicit ts_f_meta_data(std::filesystem::path path);
        meta_data_handle get();

    private:
        friend class meta_data_handle;

        void unlock();

        std::mutex m_mutex;
        uh::uhv::f_meta_data m_file_meta;
    };

// ---------------------------------------------------------------------

} // namespace uh::uhv