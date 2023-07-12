# C SDK - Connection Startup & Shutdown

## CREATE

Creates an instance of `UDB_CONNECTION` which can be used to perform various
`UDB` operations such as adding and getting documents tofrom the `UDB` database.
 - `instance`: `UDB` instance to use in order to create a connection to the database
 - `UDB_CONNECTION*`: pointer to the connection instance

```c++
UDB_CONNECTION* udb_create_connection(UDB* instance);
```
-----


## PING

 Check if database connection is still alive.

 - `conn`: `UDB` connection which can be used to perform various `UDB` operations.
 - `UDB_RESULT`: an enum which describes the result of the operation.

```c++
UDB_RESULT udb_ping(UDB_CONNECTION conn);
```

-----

## DESTROY

Destroys an instance of `UDB_CONNECTION` created via udb_create_connection.
 - `conn`: `UDB_CONNECTION` instance to deallocate

```c++
UDB_RESULT udb_destroy_connection(UDB_CONNECTION* conn);
```