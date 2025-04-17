# Overview

This document defines the etcd directory structure implemented by UltiHash
cluster.


## Variables

The following variables will be used in this document:

- `<namespace>` the namespace assigned to the cluster, defaults to `uh`
- `<service_class>` specifies a type of service, possible values being `storage`,
  `deduplicator`, `coordinator`, `entrypoint`
- `<service_id>` the (numeric) id of a service, currently a serial number

# ETCD manager watchdog

`<namespace>/watchdog/` \
    monitored by all services to check for etcd connectivity.

## ETCD manager watchdog

`<namespace>/watchdog/` \
    monitored by all services to check for etcd connectivity.


## Service announcements

Services announcing their availability for other cluster members through etcd
key prefixes. Announcements are assigned a TTL and are deleted automatically
when it expires. There is no guarantee that the announced service is actually
available.

`/<namespace>/services/<service_class>/announced/<service_id>` \
    when defined, a service of class `<service_class>` and service id
    `<service_id>` is available with details being stored under the path
  `/<namespace>/services/<service_class>/attributes/<service_id>`.

`/<namespace>/services/<service_class>/attributes/<service_id>/endpoint_host` \
    contains the host the service is running on.

`/<namespace>/services/<service_class>/attributes/<service_id>/endpoint_port` \
    contains the port the service is using for communication.

**TBD**: `/<namespace>/storage-groups/<storage_group_id>/storages/<service_id>` \
  contains state of storage services: 0(NEW), 1(ASSIGNED). \
  This key is watched by `coordinator`.

**TBD**: `/<namespace>/storage-groups/<storage_group_id>/initialized` \
  exists only when the storage group succeeded to assign all storage services. \
  This key has no ttl. `coordinator` reads this key.

**TBD**: `/<namespace>/storage-groups/state/<storage_group_id>` \
  contains state of group and storage services state that storage group manager
  refered, like `<group_state>,220101`, 2 means that storage is DOWN, `group_state`
  will be a single digit number also. This key is watched by `global_data_view`.

## Configuration parameters

**TBD**: `/<namespace>/storage-groups/config/<storage_group_id>`
  contains configuration of storage groups. `coordinator` writes this key.

**TBD**: `/<namespace>/storage-groups/assignments/<service_id>` \
  contains the storage group ID to which a storage service belongs. \
  The `coordinator` writes this key, and each storage service reads it.

## Service ID

`/<namespace>/config/class/cluster/lock` \
    used during service ID allocation to synchronize service access to ids.

`/<namespace>/config/class/cluster/current_id/<service_class>` \
    next ID value for services of type `<service_class>`.

`/<namespace>/config/class/storage/lock` \
used during storage service ID registration to synchronize access to registered ids.

`/<namespace>/config/class/storage/registered_ids/<service_id>` \
denotes registered storage `<service_id>`s to ensure that no two service instances are started with the same `<service_id>`.

## Cluster License

`/<namespace>/config/license` \
    current cluster license as JSON string. The license has been verified.
