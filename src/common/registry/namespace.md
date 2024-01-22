# Naming convention for etcd resources
There are three major types of resources we want to distinguish:
* Service announcements
* Configuration parameters
* State

## Service announcements
Using the `service_registry` class, each service instance announces its availability by setting the announced keys as follows: 

```
/<namespace>/services/announced/<service_class>/<service_id>
```
The purpose of this key is to indicate that a certain service is available and is helpful to use it as a prefix to listen to since any changes to this key
means that a service was either added or removed. Example of storage service with index 0 and 1 being announced in etcd:
```
/uh/services/announced/storage/0
/uh/services/announced/storage/1
```

Each service has runtime variables that needs to be exposed in the etcd such that other services can get the information like the hostname and the port of the service.
These runtime variables are exposed by the `/attributes/` path as follows:

```
/<namespace>/services/attributes/<service_class>/<service_id>/endpoint_host
/<namespace>/services/attributes/<service_class>/<service_id>/endpoint_host
/<namespace>/services/attributes/<service_class>/<service_id>/endpoint_port
/<namespace>/services/attributes/<service_class>/<service_id>/<optional: further parameters>
```
Each service instance *must* provide the exposed attributes `endpoint_host` and `endpoint_port`. 
Optionally, services may choose to export further attributes.

Example of entries representing a cluster comprised of 2 storage instances, 1 deduplicator instance, 1 directory instance, and 2 entrypoint instances:
```
/uh/services/attributes/storage/0/endpoint_host -> storage0.ultihash.io
/uh/services/attributes/storage/0/endoint_port  -> 9200
/uh/services/attributes/storage/1/endpoint_host -> storage1.ultihash.io
/uh/services/attributes/storage/1/endoint_port  -> 9200
/uh/services/attributes/deduplicator/0/endpoint_host -> dedupe0.ultihash.io
/uh/services/attributes/deduplicator/0/endoint_port  -> 9300
/uh/services/attributes/directory/0/endpoint_host -> dir0.ultihash.io
/uh/services/attributes/directory/0/endoint_port  -> 9400
/uh/services/attributes/entrypoint/0/endpoint_host -> ep0.ultihash.io
/uh/services/attributes/entrypoint/0/endoint_port  -> 8080
/uh/services/attributes/entrypoint/1/endpoint_host -> ep1.ultihash.io
/uh/services/attributes/entrypoint/1/endoint_port  -> 8080
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


## State
Using the `state_registry` class (not yet available), services can persist their state on two scopes:
1. state that applies to all instances of a service `class` and
2. state that applies to a specific `instance` of a service class.

```
/<namespace>/state/class/<service_class>/<state_descriptor>
/<namespace>/state/instance/<service_class>/<service_id>/<state_descriptor>
```

# Reacting to cluster changes
## Watcher Class
`service_registry` has a `watcher` member variable which is responsible for watching any changes in etcd. 
When there is some change in the etcd server, the `watcher` client in the `service_registry` calls a handler function
which we assign. This handler function gets `etcd Response` object responsible for communicating what happened in the
etcd server. This then allows us to call the registered functions from the handler function which can be used to react
to addition/removal of any services.

## Registering callback functions
We can register callback functions to be called in order to add/remove clients to the added/removed services. `services` 
class is responsible for registering such callbacks. The purpose of `services` class is to maintain all the
client connections to the required services. Thus, it is `service` class's responsibility to register callbacks 
which allows a mechanism to add/remove clients accordingly to the services' addition/removal. 


All further details need to be fleshed out alongside the implementation of the `state_registry` class.