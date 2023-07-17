//
// Created by masi on 7/17/23.
//

#ifndef CORE_CLUSTER_CONFIG_H
#define CORE_CLUSTER_CONFIG_H

namespace uh::cluster {

struct cluster_config {
    int data_node_jobs;
    int dedupe_jobs;
    int redupe_jobs;
    int phonebook_jobs;
    int entry_jobs;
};

} // end namespace uh::cluster

#endif //CORE_CLUSTER_CONFIG_H
