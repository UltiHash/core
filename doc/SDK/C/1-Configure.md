# Configuration

## CREATE

Creates an instance of `UDB_CONFIG` which can be used to put configuration parameters.
 - `UDB_CONFIG*`: pointer to the configuration where parameters are set
```c++
UDB_CONFIG* udb_create_config();
```


Creates an instance of `UDB` with the given `UDB_CONFIG` instance.
 - `cfg`: `UDB_CONFIG` instance where the configuration parameters are set
 - `UDB*`: pointer to the UDB structure created
```c++
UDB* udb_create_instance(UDB_CONFIG *cfg);
```

-----

## SET

Sets the connection parameters of the host node in the given config file.
 - `cfg`: config structure where the connection parameters can be set
 - `hostname`: Name of the host to connect to
 - `port`: Port on which the host is running
 - `UDB_RESULT`: enum that describes the result of the operation
```c++
UDB_RESULT udb_config_set_host_node(UDB_CONFIG cfg, const char hostname, uint16_t port);
```
-----

## DESTROY

Deallocates the instance created from udb_create_config.
 - `cfg`: config structure where the connection parameters can be set
 - `UDB_RESULT`: enum that describes the result of the operation
```c++
UDB_RESULT udb_destroy_config(UDB_CONFIG *cfg);
```


Destroys an instance of `UDB` created via `udb_create_instance`
 - `udb_instance`: `UDB` instance to destroy
```c++
UDB_RESULT udb_destroy_instance(UDB* udb_instance);
```

---

## Version

```c++
const char* get_sdk_name();
```

```c++
const char* get_sdk_version();
```