//
// Created by masi on 7/28/23.
//
#include "common.h"
#include <stdexcept>
#include <string>
#include <cstring>
#include <boost/interprocess/mapped_region.hpp>

namespace uh::cluster {

    void *align_ptr(void *ptr) noexcept {
        static const size_t PAGE_SIZE = boost::interprocess::mapped_region::get_page_size();
        return static_cast <char*> (ptr) - reinterpret_cast <size_t> (ptr) % PAGE_SIZE;
    }

    void sync_ptr (void *ptr, std::size_t size) {
        /*
        const auto aligned = align_ptr (ptr);
        if (msync(aligned, static_cast <char*> (ptr) - static_cast <char*> (aligned) + size, MS_SYNC) != 0) {
            throw std::system_error (errno, std::system_category(), "could not sync the mmap data");
        }
         */
    }


} // end namespace uh::cluster
