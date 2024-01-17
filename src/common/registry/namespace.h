//
// Created by max on 17.01.24.
//

#ifndef UH_CLUSTER_NAMESPACE_H
#define UH_CLUSTER_NAMESPACE_H
namespace uh::cluster {

static constexpr const char * m_etcd_default_key_prefix = "/uh/";
static constexpr const char * m_etcd_global_key_prefix = "/uh/global/";
static constexpr const char * m_etcd_lock_key = "/uh/global/lock";
static constexpr const char * m_etcd_initialized_key = "/uh/global/initialized";


}

#endif //UH_CLUSTER_NAMESPACE_H
