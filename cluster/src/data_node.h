//
// Created by masi on 7/17/23.
//

#ifndef CORE_DATA_NODE_H
#define CORE_DATA_NODE_H

#include <functional>
#include <iostream>
#include "cluster_config.h"
#include "data_store.h"
#include <mpi.h>

namespace uh::cluster {
class data_node {
public:

    data_node (data_store_config conf, int id):
            m_job_name ("data_node_" + std::to_string (id)),
            m_data_store (std::move (conf), id)
    {}

    void run() {
        std::cout << "hello from " << m_job_name << std::endl;
        MPI_Status status;
        int message_size;


        while (!m_stop) {

            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            switch (status.MPI_TAG) {
                case message_types::INIT_REQ:
                    handle_init(status.MPI_SOURCE);
                    break;
                case message_types::WRITE_REQ:
                    MPI_Get_count(&status, MPI_CHAR, &message_size);
                    handle_write(status.MPI_SOURCE, message_size);
                    break;
                case message_types::READ_REQ:
                    MPI_Get_count(&status, MPI_CHAR, &message_size);
                    handle_read(status.MPI_SOURCE, message_size);
                    break;
            }


        }
    }

    void handle_init (int target) const {
        MPI_Status status;
        int dummy;
        MPI_Recv (&dummy, 1, MPI_INT, target, message_types::INIT_REQ, MPI_COMM_WORLD, &status);

        const auto id = m_data_store.get_data_id();
        const auto rc = MPI_Send(&id, 1, MPI_INT, target, message_types::INIT_RESP, MPI_COMM_WORLD);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not send init response");
        }
    }

    void handle_write (int source, int size) {
        MPI_Status status;
        const auto buf = std::make_unique_for_overwrite<char []>(size);
        auto rc = MPI_Recv (buf.get(), size, MPI_CHAR, source, message_types::WRITE_REQ, MPI_COMM_WORLD, &status);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not receive the data to be written");
        }
        const auto addr = m_data_store.write ({buf.get(), static_cast <size_t>(size)});
        const auto address_size = static_cast <int> (addr.size * sizeof (wide_span));
        rc = MPI_Send(addr.data.get(), address_size, MPI_CHAR, source, message_types::WRITE_RESP, MPI_COMM_WORLD);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not send the address of the written data");
        }
    }

    void handle_read (int source, int req_size) const {
        MPI_Status status;
        address addr(req_size / sizeof(wide_span));
        auto rc = MPI_Recv(addr.data.get(), req_size, MPI_CHAR, source, message_types::READ_REQ, MPI_COMM_WORLD,
                           &status);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not receive the data address for read operation");
        }
        std::size_t read_size = 0;
        for (int i = 0; i < addr.size; ++i) {
            read_size += addr.data [i].size;
        }

        const auto buffer = std::make_unique_for_overwrite <char []>(read_size + sizeof (size_t));
        size_t& data_size = *reinterpret_cast <size_t*> (buffer.get());
        data_size = read_size;
        size_t tr = sizeof (size_t);
        for (int i = 0; i < addr.size; ++i) {
            const auto r = m_data_store.read(buffer.get() + tr, addr.data[i].pointer, addr.data[i].size);
            if (r != addr.data[i].size) [[unlikely]] {
                throw std::out_of_range ("NThe requested data is not available");
            }
            tr += r;
        }

        size_t w = 0;
        constexpr auto max_int = static_cast <size_t> (std::numeric_limits <int>::max());
        while (w < read_size + sizeof (size_t)) {
            const auto send_size = static_cast <int> (std::min (max_int, read_size - w));
            rc = MPI_Send(buffer.get() + w, send_size, MPI_CHAR, source, message_types::READ_RESP, MPI_COMM_WORLD);
            if (rc != MPI_SUCCESS) [[unlikely]] {
                throw std::runtime_error("Could not send the read data");
            }
            w += send_size;
        }

    }

    const std::string m_job_name;
    uh::cluster::data_store m_data_store;
    bool m_stop = false;

};
} // end namespace uh::cluster
#endif //CORE_DATA_NODE_H
