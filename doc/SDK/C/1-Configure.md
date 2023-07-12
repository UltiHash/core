# C SDK - Configuration


The following functions are used to configure the connections to the host and the db instances.

---


Opaque structure that holds the underlying `UDB` instance.

Allocated and initialized with `udb_create_instance`.
Cleaned up and deallocated with `udb_destroy_instance`.


```c++
typedef struct UDB_STATE_STRUCT UDB;
```

---

Opaque structure that holds the configuration parameters to create an instance of
`UDB`.

Allocated and initialized with `udb_create_config`.
Cleaned up and deallocated with `udb_destroy_config`.

```c++
typedef struct UDB_CONFIG_STRUCT UDB_CONFIG;
```

---

Creates an instance of `UDB_CONFIG` which can be used to put configuration parameters.

The returned pointer to the `UDB_CONFIG` structure has to be deallocated using
`udb_destroy_config` as it is allocated on `udb_create_config` function call.

 - returns pointer to the `UDB_CONFIG` structure created. On error `nullptr` is returned.

```c++
UDB_CONFIG* udb_create_config();
```

---

Deallocates the instance of `UDB_CONFIG` created from `udb_create_config`.

 - `config`: `UDB_CONFIG` instance to be deallocated
 - returns `enum` that describes the result of the operation

```c++
UDB_RESULT udb_destroy_config(UDB_CONFIG* config);
```

---

Sets the connection parameters of the host node in the given `UDB_CONFIG` instance.

 - `cfg`: config structure where the connection parameters can be set
 - `hostname`: Name of the host to connect to
 - `port`: Port on which the host is running
 - `returns`: enum that describes the result of the operation

```c++
UDB_RESULT udb_config_set_host_node(UDB_CONFIG* cfg, const char *hostname, uint16_t port);
```

---

Creates an instance of `UDB` with the given `UDB_CONFIG` instance.

The instance of the `UDB` created has to be deallocated using `udb_destroy_instance`.

 - `config` `UDB_CONFIG` instance where the configuration parameters are set
 - returns pointer to the `UDB` structure created. On error `nullptr` is returned.

```c++
UDB* udb_create_instance(UDB_CONFIG *config);
```

---

Destroys an instance of `UDB` created via `udb_create_instance`.

 - `udb_instance`: `UDB` instance to destroy
 - returns `enum` that describes the result of the operation

```c++
UDB_RESULT udb_destroy_instance(UDB* udb_instance);
```

---