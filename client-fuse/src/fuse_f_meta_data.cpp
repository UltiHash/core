#include <fuse_f_meta_data.h>

namespace uh::uhv {

// ---------------------------------------------------------------------

ts_f_meta_data::meta_data_handle::~meta_data_handle()
{
    m_ts_fmeta_class.unlock();
}

// ---------------------------------------------------------------------

uh::uhv::f_meta_data& ts_f_meta_data::meta_data_handle::operator()()
{
    return m_file_meta;
}

// ---------------------------------------------------------------------

ts_f_meta_data::meta_data_handle::meta_data_handle(ts_f_meta_data &ts_fmeta_class, uh::uhv::f_meta_data &file_meta) :
        m_ts_fmeta_class(ts_fmeta_class), m_file_meta(file_meta)
{
    // empty
}

// ---------------------------------------------------------------------

ts_f_meta_data::ts_f_meta_data(uh::uhv::f_meta_data file_meta_data): 
    m_file_meta (std::move (file_meta_data))
{
}

// ---------------------------------------------------------------------

ts_f_meta_data::meta_data_handle ts_f_meta_data::get()
{
    m_mutex.lock();
    return {*this, m_file_meta};
}

// ---------------------------------------------------------------------

uh::uhv::f_meta_data& ts_f_meta_data::n_ts_get()
{
    return m_file_meta;
}

// ---------------------------------------------------------------------

void ts_f_meta_data::unlock()
{
    m_mutex.unlock();
}

// ---------------------------------------------------------------------

} // namespace uh::uhv
