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

TBD: `/<namespace>/<service_class>/hostports/<service_id>` \
  contains the host/port of the service is running on. coordinator, deduplicator,
  entrypoint services support this key.

`/<namespace>/storage_groups/temporaries/<storage_group_id>/storage_offsets/<storage_id>` \
  contains storage offset of all storage services. This key is watched by  a 
  leader.

`/<namespace>/storage_groups/<storage_group_id>/leader` \
  contains leader's storage id. It can contain empty value during election as a 
  temporary value.

`/<namespace>/storage_groups/<storage_group_id>/storage_states/<storage_id>` \
  contains state of storage services: DOWN, NEW, and ASSIGNED as a number. \
  This key is watched by a leader.

`/<namespace>/storage_groups/<storage_group_id>/group_initialized` \
  exists only when the storage group succeeded to assign all storage services. \
  This key has no ttl. A leader reads this key.

`/<namespace>/storage_groups/<storage_group_id>/group_state` \
  contains state of group. This key is watched by group data view.

`/<namespace>/storage_groups/<storage_group_id>/storage_hostports/<storage_id>` \
  contains hostport of storages that group data view refers, like `hostname:8080`.

## Configuration parameters

`/<namespace>/storage_groups/group_configs/<storage_group_id>`
  contains configuration of storage groups. `coordinator` writes this key.

`/<namespace>/storage_groups/storage_assignments/<storage_id>` \
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
