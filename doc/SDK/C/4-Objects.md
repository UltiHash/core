# C API - Objects

Opaque structure that represents the concept of an "object". An object is a
structure that holds a key, value, and labels.

Allocated and initialized with `udb_init_object`.
Cleaned up and deallocated with `udb_destroy_object`.

typedef struct UDB_OBJECT_STRUCT UDB_OBJECT;


A wrapper struct that wraps a `char*`` and the size of the pointed data.

It is used to wrap the keys and values of the objects when `getting` from objects for convenience purposes.

```c++
struct UDB_DATA
{
   char* data;
   size_t size;

   UDB_DATA(char* rec_ptr, size_t rec_size) :
           data(rec_ptr), size(rec_size)
   {}

   UDB_DATA() : data(nullptr), size(0) {}

};
```

## Create


Creates an instance of `UDB_OBJECT` structure.

The allocated instance of `UDB_OBJECT` has to be deallocated with `udb_destroy_object`.

 - `key`: pointer to the array of key characters
 - `key_size`: size of the key
 - `value`: pointer to the array of value characters
 - `value_size`: size of the value
 - `labels`: char** which holds pointers to the labels. The label strings themselves are null terminated.
 - `label_count`: Number of labels the user wants to assign for the value
 - returns pointer to the `UDB_OBJECT` allocated. On error `nullptr` is returned.

```c++
UDB_OBJECT* udb_init_object(char* key, size_t key_size, char* value, size_t value_size,
                               char** labels, size_t label_count);
```

## Destroy


Deallocates the instance of `UDB_OBJECT` structure created via `udb_init_object`.

 - `obj`: object to deallocate
 - returns UDB_RESULT enum that describes the result of the operation

```c++
UDB_RESULT udb_destroy_object(UDB_OBJECT* obj);
```



Deallocates the `UDB_DATA` struct which was allocated before.

 - `data`: pointer to the `UDB_DATA` struct
 - returns `enum` that describes the result of the operation

```c++
UDB_RESULT udb_destroy_udb_data(UDB_DATA* data);
```

## Getters


Gets a pointer to the `UDB_DATA` struct which holds the key information. The struct is allocated and has to
be deallocated with `udb_destroy_udb_data` function.

 - `obj`: pointer to the `UDB_OBJECT` struct
 - returns a pointer to `UDB_DATA` which holds the value information i.e. `char*` and `size_t`.
On error `nullptr` is returned.

```c++
UDB_DATA* udb_get_key(UDB_OBJECT* obj);
```

Gets the value of the object.
Returns a pointer to `UDB_DATA` which holds the value information i.e. `char*` and `size_t`

 - `obj`: pointer to the `UDB_OBJECT` struct
 - returns a pointer to `UDB_DATA` which holds the value information i.e. `char*` and `size_t`.
On error `nullptr` is returned.

```c++
UDB_DATA* udb_get_value(UDB_OBJECT* obj);
```

Gets the count of the labels inside the object.

 - `obj`: pointer to the `UDB_OBJECT` struct
 - returns number of labels that is in the object

```c++
size_t udb_get_labels_count(UDB_OBJECT* obj);
```

Gets the pointer to the null-terminated label string from the given index.

 - `obj`: pointer to the object
 - `label_index`: index of the label string which is to be retrieved
 - `returns pointer`: to the null-terminated label string at the given index.
On error `nullptr` is returned.

```c++
char* udb_get_label(UDB_OBJECT* obj, size_t label_index);
```