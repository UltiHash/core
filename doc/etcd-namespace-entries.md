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
  `deduplicator`, `coordinator`, `entrypoint`
- `<service_id>` the (numeric) id of a service, currently a serial number

# ETCD manager watchdog

`<namespace>/watchdog/` \
    monitored by all services to check for etcd connectivity.

# Service announcements

Services announcing their availability for other cluster members through etcd
key prefixes. Announcements are assigned a TTL and are deleted automatically
when it expires. There is no guarantee that the announced service is actually
available.

`/<namespace>/services/<service_class>/announced/<service_id>` \
    when defined, a service of class `<service_class>` and service id
    `<service_id>` is available with details being stored under the path
  `/<namespace>/services/<service_class>/attributes/<service_id>`

`/<namespace>/services/<service_class>/attributes/<service_id>/endpoint_host` \
    contains the host the service is running on

`/<namespace>/services/<service_class>/attributes/<service_id>/endpoint_port` \
    contains the port the service is using for communication

# Service ID

`/<namespace>/config/class/cluster/lock` \
    used during service ID allocation to synchronize service access to ids

`/<namespace>/config/class/cluster/current_id/<service_class>` \
    next ID value for services of type `<service_class>`

# Cluster License

`/<namespace>/config/license` \
    current cluster license as JSON string. The license has been verified.
