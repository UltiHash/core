//
// Created by max on 17.01.24.
//

#ifndef UH_CLUSTER_NAMESPACE_H
#define UH_CLUSTER_NAMESPACE_H

#define NAMESPACE "uh"

namespace uh::cluster {

static constexpr const char* etcd_global_config_key_prefix      = "/" NAMESPACE "/config/class/";
static constexpr const char* etcd_instance_config_key_prefix    = "/" NAMESPACE "/config/instance/";
static constexpr const char* etcd_global_state_key_prefix       = "/" NAMESPACE "/state/class/";
static constexpr const char* etcd_instance_state_key_prefix     = "/" NAMESPACE "/state/instance/";
static constexpr const char* etcd_services_announced_key_prefix           = "/" NAMESPACE "/services/announced/";
static constexpr const char* etcd_services_attributes_key_prefix           = "/" NAMESPACE "/services/attributes/";

static constexpr const char* etcd_global_lock_key    = "/" NAMESPACE "/config/class/cluster/lock";
static constexpr const char* etcd_initialized_key    = "/" NAMESPACE "/config/class/cluster/initialized";

}

#endif //UH_CLUSTER_NAMESPACE_H
