# Naming convention for etcd resources
There are three major types of resources we want to distinguish:
* Service announcements
* Configuration parameters
* State

## Service announcements
Using the `service_registry` class, each service instance announces its availability by setting temporary keys as follows:
```
/<namespace>/services/<service_class>/<service_id>/endpoint_host
/<namespace>/services/<service_class>/<service_id>/endpoint_port
/<namespace>/services/<service_class>/<service_id>/<optional: further parameters>
```
Each service instance *must* provide the exposed attributes `endpoint_host` and `endpoint_port`. 
Optionally, services may choose to export further attributes.

Example of entries representing a cluster comprised of 2 storage instances, 1 deduplicator instance, 1 directory instance, and 2 entrypoint instances:
```
/uh/services/storage/0/endpoint_host -> storage0.ultihash.io
/uh/services/storage/0/endoint_port  -> 9200
/uh/services/storage/1/endpoint_host -> storage1.ultihash.io
/uh/services/storage/1/endoint_port  -> 9200
/uh/services/deduplicator/0/endpoint_host -> dedupe0.ultihash.io
/uh/services/deduplicator/0/endoint_port  -> 9300
/uh/services/directory/0/endpoint_host -> dir0.ultihash.io
/uh/services/directory/0/endoint_port  -> 9400
/uh/services/entrypoint/0/endpoint_host -> ep0.ultihash.io
/uh/services/entrypoint/0/endoint_port  -> 8080
/uh/services/entrypoint/1/endpoint_host -> ep1.ultihash.io
/uh/services/entrypoint/1/endoint_port  -> 8080
```
All entries in the services hierarchy *must* be temporary values that are set using a time-to-live (TTL) to make sure the likelihood of stale entries is as little as possible.

## Configuration parameters
Using the `config_registry` class, configuration parameters can be stored and retrieved on two different scopes: 
1. configuration parameters that apply to all instances of a service `class` and
2. configuration parameters that apply to a specific `instance` of a service class.

`class`-specific configuration parameters are used to specify configuration parameters that in general apply to all `instances` of the specified service `class`.
`instance`-specific configuration parameters can be used to override configuration parameters for a specific `instance`:
```
/<namespace>/config/class/<service_class>/<config_parameter_key>
/<namespace>/config/instance/<service_class>/<service_id>/<config_parameter_key>
```
For the retrieval of configuration parameters, `instance`-specific parameters take precedence over `class`-specific parameters.
If for a requested configuration parameter no `instance`-specific key is available, the `class`-specific key is used instead.

Example of storage instances overwriting the directory config parameter to avoid using the same directory in a shared file system:
```
/uh/config/class/storage/directory -> /var/uh/storage/
/uh/config/instance/storage/0/directory -> /var/uh/storage/0/
/uh/config/instance/storage/1/directory -> /var/uh/storage/1/
```
All entries in the configuration hierarchy *must* be persistent values.

### Regular configuration parameters
Regular configuration parameters, as defined in `common/utils/common.h`:

| enum-option          | key                 | scope          |
|:---------------------|:--------------------|:---------------|
| CFG_SERVER_THREADS   | server_threads      | class/instance | 
| CFG_SERVER_BIND_ADDR | server_bind_address | class/instance |
| CFG_SERVER_PORT      | server_port         | class/instance |


### Special cases

#### cluster` service class

The `cluster` value is a special case of `<service_class> and can only be used on the class scope to maintain cluster-wide configuration parameters.
Currently, it is used to determine if a cluster has already been initialized, or if this is the first time any service of the cluster is started:
```
/<namespace>/config/class/cluster/initialized
```

#### volatile lock key
The `lock` key is used to implement a locking mechanism across all service classes.
Currently, this mechanism is used to avoid race conditions regarding the initialization of the cluster.
The `lock` key is a temporary value and expires based on a TTL in case of an unorderly shutdown.
```
/<namespace>/config/class/cluster/lock
```

#### automatic id generation
For each service class, a counter value is maintained to indicate the highest, currently used id for that service class.
```
/<namespace>/config/class/cluster/current_id/storage -> 0
/<namespace>/config/class/cluster/current_id/deduplicator -> 0
/<namespace>/config/class/cluster/current_id/directory -> 0
/<namespace>/config/class/cluster/current_id/entrypoint -> 0
```

## State
Using the `state_registry` class (not yet available), services can persist their state on two scopes:
1. state that applies to all instances of a service `class` and
2. state that applies to a specific `instance` of a service class.

```
/<namespace>/state/class/<service_class>/<state_descriptor>
/<namespace>/state/instance/<service_class>/<service_id>/<state_descriptor>
```

All further details need to be fleshed out alongside the implementation of the `state_registry` class.