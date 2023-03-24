#ifndef FUSE_F_META_DATA_H 
#define FUSE_F_META_DATA_H

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

        ts_f_meta_data(uh::uhv::f_meta_data m_file_meta);
        meta_data_handle get();
        uh::uhv::f_meta_data& n_ts_get();

    private:
        friend class meta_data_handle;

        void unlock();

        std::mutex m_mutex;
        uh::uhv::f_meta_data m_file_meta;
    };

// ---------------------------------------------------------------------

} // namespace uh::uhv

#endif // FUSE_F_META_DATA_H
