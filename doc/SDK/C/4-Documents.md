# C API - Documents

## CREATE 

 Creates an instance of `UDB_DOCUMENT` structure and returns a pointer it
 if the allocation was successful. On failure, a `nullptr` is returned.

 - `key` UDB_DATA which holds the key information.
 - `value` UDB_DATA which holds the value information.
 - `labels` char which holds pointers to the labels. The labels themselves are null terminated.
 - `label_count` Number of labels the user wants to assign for the value.
  - Returns a pointer to the `UDB_DOCUMENT` allocated. On failure a `nullptr` is returned.

```c++
UDB_DOCUMENT* udb_init_document(char* key, size_t key_size, char* value, size_t value_size, char** labels, size_t label_count);
```

## DESTROY

Deallocates the instance of `UDB_DOCUMENT` structure. If the document instance was set to have an
owning enum through `udb`, it recursively deallocates references to all the object it holds, else it only
deallocates itself.

 - `doc` document to deallocate.
 - Returns an `UDB_RESULT` that describes the result of the operation.

```c++
UDB_RESULT udb_destroy_document(UDB_DOCUMENT** ptr_to_document_ptr);
```
## GETTERS 

```c++
UDB_DATA* udb_get_key(UDB_DOCUMENT* doc);
```
```c++
UDB_DATA* udb_get_value(UDB_DOCUMENT* doc);
```
```c++
size_t udb_get_labels_count(UDB_DOCUMENT* doc);
```
```c++
char* udb_get_label(UDB_DOCUMENT* doc, size_t label_index);
```