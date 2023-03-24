#include "fuse_ts_container.h"

namespace uh::uhv {

// ---------------------------------------------------------------------

ts_container::container_handle::~container_handle()
{
    m_container.unlock();
}

// ---------------------------------------------------------------------

std::unordered_map <std::string, uh::uhv::f_meta_data>& ts_container::container_handle::operator()()
{
    return m_paths_metadata;
}

// ---------------------------------------------------------------------

ts_container::container_handle::container_handle(ts_container &container,
                                                 std::unordered_map<std::string, uh::uhv::f_meta_data> &paths) :
                                                 m_container(container), m_paths_metadata(paths)
{
}

// ---------------------------------------------------------------------

ts_container::container_handle ts_container::get()
{
    m_mutex.lock();
    return {*this, m_paths_metadata};
}

// ---------------------------------------------------------------------

std::unordered_map <std::string, uh::uhv::f_meta_data>& ts_container::n_ts_get()
{
    return m_paths_metadata;
}

// ---------------------------------------------------------------------

void ts_container::unlock()
{
    m_mutex.unlock();
}

// ---------------------------------------------------------------------

} // namespace uh::uhv