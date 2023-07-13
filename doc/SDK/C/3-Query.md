# C SDK - Query


##  WRITE QUERY



Opaque structure that describes a write query for writing objects into the database.

Provided functions can be used to fill up this query structure and subsequently write
objects to the database.

Allocated and initialized with `udb_create_write_query`.
Cleaned up and deallocated with `udb_destroy_write_query.

```c++
typedef struct UDB_OBJECTS UDB_WRITE_QUERY;
```


Creates a write query for putting an object into the database.

The allocated `UDB_WRITE_QUERY` has to be deallocated using `udb_destroy_write_query`.

 - returns pointer to the `UDB_WRITE_QUERY` created. On error nullptr is returned.

```c++
UDB_WRITE_QUERY* udb_create_write_query();
```


Adds an object in the write query.

 - `write_query` The `UDB_WRITE_QUERY` struct in which an object is to be added
 - `object` `UDB_OBJECT` struct to add

```c++
void udb_write_query_add_object(UDB_WRITE_QUERY* write_query, UDB_OBJECT* object);
```


Destroying the instance of write query.

 - `write_query` pointer to the allocated `UDB_WRITE_QUERY`
 - returns UDB_RESULT enum that describes the result of the operation

```c++
UDB_RESULT udb_destroy_write_query(UDB_WRITE_QUERY* write_query);
```


An opaque struct which holds the results received from `udb_put` function call.

Cleaned up and deallocated with `udb_destroy_write_query_results`.

```c++
struct UDB_WRITE_QUERY_RESULTS;
```


Destroying the instance of `UDB_WRITE_QUERY_RESULTS`.

 - `results` pointer to the `UDB_WRITE_QUERY_RESULTS`
 - returns UDB_RESULT enum that describes the result of the operation

```c++
UDB_RESULT udb_destroy_write_query_results(UDB_WRITE_QUERY_RESULTS* results);
```


Gets the number of the effective sizes returned in the `UDB_WRITE_QUERY_RESULTS`. Each object
put in the database has an effective size returned.

 - `results` pointer to the `UDB_WRITE_QUERY_RESULTS` struct
 - returns size_t count of the effective sizes returned for each object put in the database

```c++
size_t udb_get_effective_sizes_count(UDB_WRITE_QUERY_RESULTS* results);
```


Gets the effective size from the index given inside the `UDB_WRITE_QUERY_RESULTS` struct.
The effective sizes are in same order as the objects inserted in the `UDB_WRITE_QUERY`.

 - `results` pointer to the `UDB_WRITE_QUERY_RESULTS` struct
 - `index` gives the index from which to get the effective size from
 - returns uint32_t effective size of the given index from the `UDB_WRITE_QUERY_RESULTS`

```c++
uint32_t udb_get_effective_size(UDB_WRITE_QUERY_RESULTS* results, size_t index);
```


Gets the count of the return code returned in the `UDB_WRITE_QUERY_RESULTS` struct. `UDB_WRITE_QUERY_RESULTS`
struct stores an array of return codes for each objects in the write query.

 - `results` pointer to the `UDB_WRITE_QUERY_RESULTS` struct
 - returns count of the return codes returned in `UDB_WRITE_QUERY_RESULTS` struct

```c++
size_t udb_get_return_code_count(UDB_WRITE_QUERY_RESULTS* results);
```


Gets the return code at the given index. The order of return codes returned is the same as the order of the
objects added in the write query. `UDB_WRITE_QUERY_RESULTS` struct stores an array of return codes for each
objects in the write query.

 - `results` pointer to the `UDB_WRITE_QUERY_RESULTS` struct
 - `index` index from which return code is to be received
 - returns return code of each objects `put` into the database

```c++
uint8_t udb_get_return_code(UDB_WRITE_QUERY_RESULTS* results, size_t index);
```


Puts the object in the database.

 - `conn` pointer to `UDB_CONNECTION` struct
 - `write_query` pointer to the `UDB_WRITE_QUERY` struct which contains the objects to be put
in the database
 - returns pointer to the `UDB_WRITE_QUERY_RESULTS` which contains the results such as
the effective sizes and the return code of each object. On error nullptr is returned.

```c++
UDB_WRITE_QUERY_RESULTS* udb_put(UDB_CONNECTION* conn, UDB_WRITE_QUERY* write_query);
```




## READ QUERY





Opaque structure that describes a read query for reading objects from the database.

Provided functions can be used to fill up this query structure and subsequently read
objects from the database.

Allocated and initialized with `udb_create_read_query`.
Cleaned up and deallocated with `udb_destroy_read_query`.

```c++
typedef struct UDB_READ_QUERY_STRUCT UDB_READ_QUERY;
```


A structure that holds key, value and labels information.

A `UDB_READ_QUERY_RESULT` struct is retrieved from `UDB_READ_QUERY_RESULTS` struct by using the functions
provided. When the `udb_get` function is called with appropriate arguments, a pointer to the
`UDB_READ_QUERY_RESULTS` struct is returned which contains an array of `UDB_READ_QUERY_RESULT` describing the
individual result retrieved from the read query.

```c++
struct UDB_READ_QUERY_RESULT
{
   char* key;
   size_t key_size;
   char* value;
   size_t value_size;
   char** labels;
   size_t label_count;

   UDB_READ_QUERY_RESULT(char* rec_key, size_t rec_key_size, char* rec_value, size_t rec_value_size,
                         char** rec_labels, size_t rec_label_count) :
                         key(rec_key), key_size(rec_key_size), value(rec_value), value_size(rec_value_size),
                         labels(rec_labels), label_count(rec_label_count)
   {}
};
```


An opaque structure that holds all the results retrieved from the `udb_get` function.

Cleaned up and deallocated with `udb_destroy_read_query_results`.

```c++
struct UDB_READ_QUERY_RESULTS;
```


Allocates the `UDB_READ_QUERY` struct for constructing read query in order to get objects from
the database.

The allocated instance has to be deallocated with `udb_destroy_read_query`.

 - returns UDB_READ_QUERY* pointer to the `UDB_READ_QUERY` struct allocated. On error nullptr is returned.

```c++
UDB_READ_QUERY* udb_create_read_query();
```


Adds keys to the read_query.

 - `read_query` pointer to `UDB_READ_QUERY` struct in which the given key is to be added
 - `key` pointer to the array of key characters
 - `key_size` size of the key given
 - returns UDB_RESULT enum that describes the result of the operation

```c++
UDB_RESULT udb_read_query_add_key(UDB_READ_QUERY* read_query, char* key, size_t key_size);
```


Setting the labels in read_query.

 - `read_query` pointer to `UDB_READ_QUERY` struct in which to add the given labels
 - `labels` pointer to an array containing pointers to null terminated null label strings
 - `label_count` number of label strings given
 - returns UDB_RESULT enum that describes the result of the operation

```c++
UDB_RESULT udb_read_query_set_labels(UDB_READ_QUERY* read_query, char** labels, size_t label_count);
```


Destroys the allocated `UDB_READ_QUERY` struct.

 - `read_query` pointer to the allocated `UDB_READ_QUERY` struct
 - returns UDB_RESULT enum that describes the result of the operation

```c++
UDB_RESULT udb_destroy_read_query(UDB_READ_QUERY* read_query);
```


Getting the object using the `UDB_READ_QUERY` struct. Returns a pointer to the `UDB_READ_QUERY_RESULTS`
struct which contains results of the `udb_get` operation. `udb_results_next` function can be used to go through the
returned `UDB_READ_QUERY_RESULTS` struct.

 - `conn` pointer to `UDB_CONNECTION` struct
 - `read_query` pointer to the read query used to get the object from the database
 - returns UDB_READ_QUERY_RESULTS* pointer to the struct that contains the results of the `udb_get` operation.
The struct contains all the retrieved objects. On error nullptr is returned.

```c++
UDB_READ_QUERY_RESULTS* udb_get(UDB_CONNECTION* conn, UDB_READ_QUERY* read_query);
```


Gets the pointer to the next `UDB_READ_QUERY_RESULT` from the `UDB_READ_QUERY_RESULTS` struct.

 - `results_container` pointer to the `UDB_READ_QUERY_RESULTS` struct which contains the array of
`UDB_READ_QUERY_RESULT` struct
 - `result_ptr` pointer to the variable which will hold the retrieved pointer to `UDB_READ_QUERY_RESULT`
 - returns returns true if there is the next `UDB_READ_QUERY_RESULT` in the `UDB_READ_QUERY_RESULTS` struct
else it returns false. On subsequent call to `udb_results_next`, the function will start retrieving the pointer from
the beginning.


```c++
bool udb_results_next(UDB_READ_QUERY_RESULTS* results_container, UDB_READ_QUERY_RESULT** result_ptr);
```


Deallocates the previously allocated `UDB_READ_QUERY_RESULTS` struct.

 - `results` pointer to the allocated `UDB_READ_QUERY_RESULTS` struct
 - returns UDB_RESULT enum that describes the result of the operation

```c++
UDB_RESULT udb_destroy_read_query_results(UDB_READ_QUERY_RESULTS* results);
```


Gets the count of `UDB_READ_QUERY_RESULT` inside the `UDB_READ_QUERY_RESULTS` struct

 - `results` pointer to the `UDB_READ_QUERY_RESULTS` struct
 - returns size_t number of `UDB_READ_QUERY_RESULT` in the retrieved read query

```c++
size_t udb_get_results_count(UDB_READ_QUERY_RESULTS* results);
```


Gets the pointer to the `UDB_READ_QUERY_RESULT` at a given index inside the `UDB_READ_QUERY_RESULTS` struct
 - `results` pointer to the `UDB_READ_QUERY_RESULTS` struct
 - `index` position of `UDB_READ_QUERY_RESULT` inside `UDB_READ_QUERY_RESULTS` to get
 - returns pointer to the `UDB_READ_QUERY_RESULT` struct. On error nullptr is returned.

```c++
UDB_READ_QUERY_RESULT* udb_get_result(UDB_READ_QUERY_RESULTS* results, size_t index);
```
