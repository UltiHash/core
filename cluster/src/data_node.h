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
#include <atomic>

namespace uh::cluster {
class data_node {
public:

    data_node (data_store_config conf, int id);

    void run();

private:

    void handle_init (int target) const;

    void handle_write (int source, int size);

    void handle_read (int source, int req_size) const;

    void handle_sync (int target);

    void handle_remove (int target, int message_size);

    void handle_get_used (int target);

    void handle_write_many(int source, int size);

    void handle_stop(int source);

    const std::string m_job_name;
    uh::cluster::data_store m_data_store;
    std::atomic <bool> m_stop = false;

};
} // end namespace uh::cluster
#endif //CORE_DATA_NODE_H
