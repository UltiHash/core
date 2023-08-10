//
// Created by masi on 7/27/23.
//

#include "data_node.h"

namespace uh::cluster {

data_node::data_node(data_store_config conf, int id) :
        m_job_name ("data_node_" + std::to_string (id)),
        m_data_store (std::move (conf), id) {}

void data_node::run() {
    std::cout << "hello from " << m_job_name << std::endl;
    MPI_Status status;
    int message_size;

    while (!m_stop) {

        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        try {
            switch (status.MPI_TAG) {
                case message_types::INIT_REQ:
                    handle_init(status.MPI_SOURCE);
                    break;
                case message_types::WRITE_REQ:
                    MPI_Get_count(&status, MPI_CHAR, &message_size);
                    handle_write(status.MPI_SOURCE, message_size);
                    break;
                case message_types::WRITE_MANY_REQ:
                    MPI_Get_count(&status, MPI_CHAR, &message_size);
                    handle_write_many (status.MPI_SOURCE, message_size);
                    break;
                case message_types::READ_REQ:
                    MPI_Get_count(&status, MPI_CHAR, &message_size);
                    handle_read(status.MPI_SOURCE, message_size);
                    break;
                case message_types::SYNC_REQ:
                    handle_sync(status.MPI_SOURCE);
                    break;
                case message_types::REMOVE_REQ:
                    MPI_Get_count(&status, MPI_CHAR, &message_size);
                    handle_remove (status.MPI_SOURCE, message_size);
                    break;
                case message_types::USED_REQ:
                    handle_get_used (status.MPI_SOURCE);
                    break;
                case message_types::STOP:
                    handle_stop (status.MPI_SOURCE);
                    break;
                default:
                    throw std::invalid_argument ("Unknown tag");
            }
        }
        catch (std::exception& e) {
            handle_failure (m_job_name, status.MPI_SOURCE, e);
        }

    }
}

void data_node::handle_init(const int target) const {
    MPI_Status status;
    int dummy;
    auto rc = MPI_Recv (&dummy, 1, MPI_INT, target, message_types::INIT_REQ, MPI_COMM_WORLD, &status);
    if (rc != MPI_SUCCESS) [[unlikely]] {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    const auto id = m_data_store.get_data_id();
    rc = MPI_Send(&id, 1, MPI_INT, target, message_types::INIT_RESP, MPI_COMM_WORLD);
    if (rc != MPI_SUCCESS) [[unlikely]] {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
}

void data_node::handle_write(const int source, const int size) {
    MPI_Status status;
    const auto buf = std::make_unique_for_overwrite<char []> (size);
    auto rc = MPI_Recv (buf.get(), size, MPI_CHAR, source, message_types::WRITE_REQ, MPI_COMM_WORLD, &status);
    if (rc != MPI_SUCCESS) [[unlikely]] {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
    const auto addr = m_data_store.write ({buf.get(), static_cast <size_t>(size)});
    const auto address_size = static_cast <int> (addr.size () * sizeof (wide_span));
    rc = MPI_Send(addr.data(), address_size, MPI_CHAR, source, message_types::WRITE_RESP, MPI_COMM_WORLD);
    if (rc != MPI_SUCCESS) [[unlikely]] {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
}

void data_node::handle_write_many(int source, int size) {
    MPI_Status status;
    const auto buf = std::make_unique_for_overwrite<char []> (size);
    auto rc = MPI_Recv (buf.get(), size, MPI_CHAR, source, message_types::WRITE_MANY_REQ, MPI_COMM_WORLD, &status);
    if (rc != MPI_SUCCESS) [[unlikely]] {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    size_t w = 0;
    std::vector <wide_span> addresses;
    while (w < size) {
        const auto data_size = *reinterpret_cast <uint32_t *> (buf.get() + w);
        w += sizeof(uint32_t);
        const auto addr = m_data_store.write ({buf.get() + w, static_cast <size_t>(data_size)});
        addresses.insert (addresses.cend(), addr.cbegin (), addr.cend());
        w += data_size;
    }

    if (w > size) [[unlikely]] {
        throw std::overflow_error ("Write many buffer overflow");
    }

    const auto address_size = static_cast <int> (addresses.size () * sizeof (wide_span));
    rc = MPI_Send(addresses.data (), address_size, MPI_CHAR, source, message_types::WRITE_MANY_RESP, MPI_COMM_WORLD);
    if (rc != MPI_SUCCESS) [[unlikely]] {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
}

void data_node::handle_read(const int source, const int req_size) const {
    MPI_Status status;
    address addr(req_size / sizeof(wide_span));
    auto rc = MPI_Recv(addr.data (), req_size, MPI_CHAR, source, message_types::READ_REQ, MPI_COMM_WORLD, &status);
    if (rc != MPI_SUCCESS) [[unlikely]] {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
    const auto read_size = std::accumulate(addr.cbegin(), addr.cend(), 0,
                        [] (const int total, const auto& ws) {return total + ws.size;});

    if (read_size > std::numeric_limits <int>::max()) [[unlikely]] {
        throw std::overflow_error ("The read data size is too large");
    }

    const auto buffer = std::make_unique_for_overwrite <char []>(read_size);

    size_t tr = 0;
    for (const auto& ws: addr) {
        const auto r = m_data_store.read(buffer.get() + tr, ws.pointer, ws.size);
        if (r != ws.size) [[unlikely]] {
            throw std::out_of_range ("The requested data is not available");
        }
        tr += r;
    }

    rc = MPI_Send(buffer.get(), static_cast <int> (read_size), MPI_CHAR, source, message_types::READ_RESP, MPI_COMM_WORLD);
    if (rc != MPI_SUCCESS) [[unlikely]] {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    /*
    const auto buffer = std::make_unique_for_overwrite <char []>(read_size + sizeof (size_t));

    size_t tr = sizeof (size_t);
    for (int i = 0; i < addr.size; ++i) {
        const auto r = m_data_store.read(buffer.get() + tr, addr.data[i].pointer, addr.data[i].size);
        if (r != addr.data[i].size) [[unlikely]] {
            throw std::out_of_range ("NThe requested data is not available");
        }
        tr += r;
    }


    size_t& data_size = *reinterpret_cast <size_t*> (buffer.get());
    data_size = read_size;
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
    */
}

void data_node::handle_sync(const int target) {
    MPI_Status status;
    int dummy;
    auto rc = MPI_Recv (&dummy, 1, MPI_INT, target, message_types::SYNC_REQ, MPI_COMM_WORLD, &status);
    if (rc != MPI_SUCCESS) [[unlikely]] {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    m_data_store.sync();

    rc = MPI_Send(&dummy, 1, MPI_INT, target, message_types::SYNC_OK, MPI_COMM_WORLD);
    if (rc != MPI_SUCCESS) [[unlikely]] {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
}

void data_node::handle_remove(const int target, const int message_size) {
    MPI_Status status;
    address remove_span_list (message_size / sizeof(wide_span));
    auto rc = MPI_Recv (remove_span_list.data(), message_size, MPI_CHAR, target, message_types::REMOVE_REQ, MPI_COMM_WORLD, &status);
    if (rc != MPI_SUCCESS) [[unlikely]] {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    for (const auto& ws: remove_span_list) {
        m_data_store.remove(ws.pointer, ws.size);
    }

    int dummy;
    rc = MPI_Send(&dummy, 1, MPI_INT, target, message_types::REMOVE_OK, MPI_COMM_WORLD);
    if (rc != MPI_SUCCESS) [[unlikely]] {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
}

void data_node::handle_get_used (int target) {
    MPI_Status status;
    int dummy;
    MPI_Recv (&dummy, 1, MPI_INT, target, message_types::USED_REQ, MPI_COMM_WORLD, &status);
    const auto used = m_data_store.get_used_space();

    const auto rc = MPI_Send(used.get_data(), 2, MPI_UINT64_T, target, message_types::USED_RESP, MPI_COMM_WORLD);
    if (rc != MPI_SUCCESS) [[unlikely]] {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
}

void data_node::handle_stop (int source) {
    MPI_Status status;
    m_stop = true;
    int dummy;
    const auto rc = MPI_Recv (&dummy, 1, MPI_INT, source, message_types::STOP, MPI_COMM_WORLD, &status);
    if (rc != MPI_SUCCESS) [[unlikely]] {
        MPI_Abort(MPI_COMM_WORLD, rc);
    }
};
} // end namespace uh::cluster
