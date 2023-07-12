# C SDK - Connection Startup & Shutdown



Opaque structure that holds the underlying connection to the database.

Allocated and initialized with `udb_create_connection`.
Cleaned up and deallocated with `udb_destroy_connection`.

```c++
typedef struct UDB_CONNECTION_STRUCT UDB_CONNECTION;
```

## CREATE

Creates an instance of `UDB_CONNECTION` which can be used to perform various
`UDB` operations such as adding and getting objects to/from the `UDB` database.

The instance created has to be destroyed using `udb_destroy_connection`.

 - `instance`: `UDB` instance to use in order to create a connection to the database
 - returns pointer to the `UDB_CONNECTION` instance. On error nullptr is returned.

```c++
UDB_CONNECTION* udb_create_connection(UDB* instance);
```

## DESTROY

Destroys an instance of `UDB_CONNECTION` created via `udb_create_connection`.

 - conn `UDB_CONNECTION` instance to deallocate
 - returns `enum` that describes the result of the operation

```c++
UDB_RESULT udb_destroy_connection(UDB_CONNECTION* conn);
```

## PING

Check if database connection is still alive.

 - `conn`: `UDB` connection which can be used to perform various `UDB` operations.
 - returns `enum` which describes the result of the operation

```c++
UDB_RESULT udb_ping(UDB_CONNECTION* conn);
```
