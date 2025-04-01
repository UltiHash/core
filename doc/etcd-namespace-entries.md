# Overview

This document defines the etcd directory structure implemented by UltiHash
cluster.

There are three major types of resources used by the cluster, which will be
handle in the subsequent sections:
* Service announcements
* Configuration parameters

### Variables

The following variables will be used in this document:

- `<namespace>` the namespace assigned to the cluster, defaults to `uh`
- `<service_class>` specifies a type of service, possible values being `storage`,
  `deduplicator`, `directory`, `entrypoint`
- `<service_id>` the (numeric) id of a service, currently a serial number


## Service announcements

Services announcing their availability for other cluster members through etcd
key prefixes. Announcements are assigned a TTL and are deleted automatically
when it expires. There is no guarantee that the announced service is actually
available.

`/<namespace>/services/attributes/<service_class>/<service_id>` \
  when defined, a service of class `<service_class>` and service id `<service_id>`
  is available

`/<namespace>/services/attributes/<service_class>/<service_id>/ENDPOINT_HOST` \
  contains the host the service is running on

`/<namespace>/services/attributes/<service_class>/<service_id>/ENDPOINT_PORT` \
  contains the port the service is using for communication

`/<namespace>/services/attributes/<service_class>/<service_id>/ENDPOINT_PID` \
  contains the port the service is using for communication

**TBD**: `/<namespace>/storage_groups/<storage_group_id>/storages/<storage_service_id>/state` \
  contains state of storage services

**TBD**: `/<namespace>/storage_groups/<storage_group_id>/throne` \
  is used for leader election

**TBD**: `/<namespace>/storage_groups/<storage_group_id>/state` \
  contains state of group and storage services state that storage group manager refered

## Configuration parameters

**TBD**: `/<namespace>/storage_groups/<storage_group_id>configuration/`
  contains configuration of storage groups

