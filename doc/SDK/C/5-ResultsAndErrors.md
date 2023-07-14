
# C SDK - Results and Errors

## `UDB_RESULT`

The UDB_RESULT struct describes the result of API calls:
 - `UDB_RESULT_SUCCESS` - UDB operation successfully completed
 - `UDB_RESULT_ERROR` - UDB operation failed and the error is not known
 - `UDB_BUFFER_OVERFLOW` - Data couldn't be fit in the buffer given
 - `UDB_BAD_ALLOCATION` - Memory couldn't be allocated
 - `UDB_KEY_ALREADY_SET` - Cannot set key(s) as a key type has already been previously set
 - `UDB_UNINITIALIZED_KEY` - Key was not set when using ::UDB_READ_QUERY
 - `UDB_SERVER_CONNECTION_ERROR` - Server was not found.


## `ERRORS`
Gets the message of the last error occurred.

```c++
const char get_error_message();
```


Returns an enum of type `UDB_RESULT` which describes the last error occurred.

```c++
UDB_RESULT udb_get_last_error();
```