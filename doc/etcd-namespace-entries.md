# Overview

This document defines the etcd directory structure implemented by UltiHash
cluster.

There are three major types of resources used by the cluster, which will be
handle in the subsequent sections:
* Service announcements
* Configuration parameters
* Service state

## Variables

The following variables will be used in this document:

- `<namespace>` the namespace assigned to the cluster, defaults to `uh`
- `<service_class>` specifies a type of service, possible values being `storage`,
  `deduplicator`, `directory`, `entrypoint`
- `<service_id>` the (numeric) id of a service, currently a serial number


# Service announcements

Services announcing their availability for other cluster members through etcd
key prefixes. Announcements are assigned a TTL and are deleted automatically
when it expires. There is no guarantee that the announced service is actually
available.

`/<namespace>/services/announced/<service_class>/<service_id>` \
  when defined, a service of class `<service_class>` and service id `<service_id>`
  is available

`/<namespace>/services/announced/<service_class>/<service_id>/endpoint_host` \
  contains the host the service is running on

`/<namespace>/services/announced/<service_class>/<service_id>/endpoint_port` \
  contains the port the service is using for communication


# Configuration parameters

Configuration parameters can be defined per instance or for all services of the
given type.

Parameters stored under `/<namespace>/config/class/<service_class>` are global to
by all services of that service class. Parameters stored under
`/<namespace>/config/instance/<service_class>/<service_id>/` are values local
to a single service instance (ie. `<service_class>/<service_id>`).

## Storage

`/<namespace>/config/class/storage/server_threads` \
`/<namespace>/config/instance/storage/<service_id>/server_threads` \
  number of server threads (default: 16)

`/<namespace>/config/class/storage/server_bind_address` \
`/<namespace>/config/instance/storage/<service_id>/server_bind_address` \
  network bind address (default: `IN_ADDR_ANY`)

`/<namespace>/config/class/storage/server_port` \
`/<namespace>/config/instance/storage/<service_id>/server_port` \
  service port (default: 9200)

`/<namespace>/config/class/storage/min_file_size` \
`/<namespace>/config/instance/storage/<service_id>/min_file_size` \
  minimum size of storage files created by `data_store` in bytes
  (default: 1GB)

`/<namespace>/config/class/storage/max_file_size` \
`/<namespace>/config/instance/storage/<service_id>/max_file_size` \
  maximum size of storage files created by `data_store` in bytes
  (default: 4GB)

`/<namespace>/config/class/storage/max_data_store_size` \
`/<namespace>/config/instance/storage/<service_id>/max_data_store_size` \
  maximum number of bytes stored in a `data_store` (default: `64*1024*1024*1024`)

## Deduplicator

`/<namespace>/config/class/deduplicator/server_threads` \
`/<namespace>/config/instance/deduplicator/<service_id>/server_threads` \
  number of server threads (default: 4)

`/<namespace>/config/class/deduplicator/server_bind_address` \
`/<namespace>/config/instance/deduplicator/<service_id>/server_bind_address` \
  network bind address (default: `IN_ADDR_ANY`)

`/<namespace>/config/class/deduplicator/server_port` \
`/<namespace>/config/instance/deduplicator/<service_id>/server_port` \
  service port (default: 9300)

`/<namespace>/config/class/deduplicator/min_fragment_size` \
`/<namespace>/config/instance/deduplicator/<service_id>/min_fragment_size` \
  minimum size of fragments produced by the deduplicator in bytes (default: 32)

`/<namespace>/config/class/deduplicator/max_fragment_size` \
`/<namespace>/config/instance/<service_id>/deduplicator/max_fragment_size` \
  maximum size of fragments produced by the deduplicator in bytes
  (default: 8KB)

`/<namespace>/config/class/deduplicator/worker_min_data_size` \
`/<namespace>/config/instance/<service_id>/deduplicator/worker_min_data_size` \
  TBD (default: 128KB)

`/<namespace>/config/class/deduplicator/worker_thread_count` \
`/<namespace>/config/instance/deduplicator/<service_id>/worker_thread_count` \
  number of threads used for deduplication (default: 32)

`/<namespace>/config/class/deduplicator/gdv_storage_service_connection_count` \
`/<namespace>/config/instance/deduplicator/<service_id>/gdv_storage_service_connection_count` \
  number of parallel connections per storage service (default: 16)

`/<namespace>/config/class/deduplicator/gdv_read_cache_capacity_l1` \
`/<namespace>/config/instance/deduplicator/<service_id>/gdv_read_cache_capacity_l1` \
  size of L1 cache in bytes (default: 8000000)

`/<namespace>/config/class/deduplicator/gdv_read_cache_capacity_l2` \
`/<namespace>/config/instance/deduplicator/<service_id>/gdv_read_cache_capacity_l2` \
  size of L2 cache in bytes (default: 4000)

`/<namespace>/config/class/deduplicator/gdv_l1_sample_size` \
`/<namespace>/config/instance/deduplicator/<service_id>/gdv_l1_sample_size` \
  size of L1 samples in bytes (default: 128)

`/<namespace>/config/class/deduplicator/gdv_max_data_store_size` \
`/<namespace>/config/instance/deduplicator/<service_id>/gdv_max_data_store_size` \
  maximum number of bytes stored in a `data_store` (default: 64GB)
  (deprecated)

## Directory

`/<namespace>/config/class/directory/server_threads` \
`/<namespace>/config/instance/directory/<service_id>/server_threads` \
  number of server threads (default: 4)

`/<namespace>/config/class/directory/server_bind_address` \
`/<namespace>/config/instance/directory/<service_id>/server_bind_address` \
  network bind address (default: `IN_ADDR_ANY`)

`/<namespace>/config/class/directory/server_port` \
`/<namespace>/config/instance/directory/<service_id>/server_port` \
  service port (default: 9400)

`/<namespace>/config/class/directory/min_file_size` \
`/<namespace>/config/instance/directory/<service_id>/min_file_size` \
  minimum size of files used in the *chaining data store* in bytes
  (default: 2GB)

`/<namespace>/config/class/directory/max_file_size` \
`/<namespace>/config/instance/directory/<service_id>/max_file_size` \
  maximum size of files used in the *chaining data store* in bytes
  (default: 64GB)

`/<namespace>/config/class/directory/max_storage_size` \
`/<namespace>/config/instance/directory/<service_id>/max_storage_size` \
  maximum amount of data stored in a *chaining data store* in bytes
  (default: 256GB)

`/<namespace>/config/class/directory/max_chunk_size` \
`/<namespace>/config/instance/directory/<service_id>/max_chunk_size` \
  maximum chunk size in *chaining data store* in bytes (default: 4GB)

`/<namespace>/config/class/directory/worker_thread_count` \
`/<namespace>/config/instance/directory/<service_id>/worker_thread_count` \
  number of threads used for directory tasks (default: 8)

`/<namespace>/config/class/directory/gdv_storage_service_connection_count` \
`/<namespace>/config/instance/directory/<service_id>/gdv_storage_service_connection_count` \
  number of parallel connections per storage service (default: 16)

`/<namespace>/config/class/directory/gdv_read_cache_capacity_l1` \
`/<namespace>/config/instance/directory/<service_id>/gdv_read_cache_capacity_l1` \
  size of L1 cache in bytes (default: 8000000)

`/<namespace>/config/class/directory/gdv_read_cache_capacity_l2` \
`/<namespace>/config/instance/directory/<service_id>/gdv_read_cache_capacity_l2` \
  size of L2 cache in bytes (default: 4000)

`/<namespace>/config/class/directory/gdv_l1_sample_size` \
`/<namespace>/config/instance/directory/<service_id>/gdv_l1_sample_size` \
  size of L1 samples in bytes (default: 128)

`/<namespace>/config/class/directory/gdv_max_data_store_size` \
`/<namespace>/config/instance/directory/<service_id>/gdv_max_data_store_size` \
  maximum number of bytes stored in a `data_store` (default: 64GB)
  (deprecated)

## EntryPoint

`/<namespace>/config/class/entrypoint/server_threads` \
`/<namespace>/config/instance/entrypoint/<service_id>/server_threads` \
  number of server threads (default: 4)

`/<namespace>/config/class/entrypoint/server_bind_address` \
`/<namespace>/config/instance/entrypoint/<service_id>/server_bind_address` \
  network bind address (default: `IN_ADDR_ANY`)

`/<namespace>/config/class/entrypoint/server_port` \
`/<namespace>/config/instance/entrypoint/<service_id>/server_port` \
  service port (default: 8080)

`/<namespace>/config/class/entrypoint/dedup_service_connection_count` \
`/<namespace>/config/instance/entrypoint/<service_id>/dedup_service_connection_count` \
  number of connections to deduplication service (default: 4)

`/<namespace>/config/class/entrypoint/dir_service_connection_count` \
`/<namespace>/config/instance/entrypoint/<service_id>/dir_service_connection_count` \
  number of connections to directory service (default: 4)

`/<namespace>/config/class/entrypoint/worker_thread_count` \
`/<namespace>/config/instance/entrypoint/<service_id>/worker_thread_count` \
  number of worker threads for entrypoint (default: 12)


## Cluster-wide configuration

`/<namespace>/config/class/cluster/initialized` \
  if exists, the cluster has been successfully initialized

`/<namespace>/config/class/cluster/lock` \
  if exists, access to the cluster is locked

`/<namespace>/config/class/cluster/current_id/<service_class>` \
  contains the currently highest service id for `<service_class>`

# Service State

Not yet implemented: services may propagate service state under the key
`/<namespace>/state/class/<service_class>/`
`/<namespace>/state/instance/<service_class>/<service_id>/`
