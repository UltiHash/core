//
// Created by masi on 7/28/23.
//
#include "common.h"
#include <mpi.h>
#include <stdexcept>
#include <string>
#include <cstring>
#include <boost/interprocess/mapped_region.hpp>

namespace uh::cluster {

    MPI_Comm entry_dedupe_comm;
    MPI_Comm dedupe_data_comm;
    MPI_Comm phonebook_data_comm;

    void handle_failure (const std::string& job_name, const int target, const std::exception &e) {
        const auto size = static_cast <int> (strlen (e.what()));
        const auto rc = MPI_Send(e.what(), size, MPI_CHAR, target, message_types::FAILURE, MPI_COMM_WORLD);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error(job_name + " could not inform the failure");
        }
    }


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
