# C SDK - Query

## CREATE WRITE QUERY

Creates a write query to use when putting a document.

```c++
UDB_WRITE_QUERY* udb_create_write_query();
```

## DESTROY WRITE QUERY

  Destorying the instance of write query.

```c++
UDB_RESULT udb_destroy_write_query(UDB_WRITE_QUERY** write_query_ptr_container);

UDB_RESULT udb_destroy_write_query_results(UDB_WRITE_QUERY_RESULTS** results);
```


```c++
UDB_RESULT udb_destroy_document(UDB_DOCUMENT** ptr_to_document_ptr);
```

## USING WRITE QUERIES

Adding a document in the write query.

```c++
void udb_write_query_add_document(UDB_WRITE_QUERY write_query, UDB_DOCUMENT document);
```

```c++
UDB_RESULT udb_get_effective_sizes_count(UDB_WRITE_QUERY_RESULTS results, size_t count);
```

```c++
UDB_RESULT udb_get_effective_size(UDB_WRITE_QUERY_RESULTS results, uint32_t value,size_t index);
```

Puts the document in the database.

```c++
UDB_WRITE_QUERY_RESULTS udb_put(UDB_CONNECTION conn, UDB_WRITE_QUERY write_query);
```



----

## CREATE READ QUERY

Creating a read query for reading documents.

```c++
UDB_READ_QUERY* udb_create_read_query();
```

## DESTROY READ QUERY

 Destroying a read_query.

```c++
UDB_RESULT udb_destroy_read_query(UDB_READ_QUERY** read_query_ptr_container);

UDB_RESULT udb_destroy_read_query_results(UDB_READ_QUERY_RESULTS** results);
```

## USING READ QUERIES

 Adding keys to the read_query.
 - `read_query`
 - `key`
 - `key_size`

```c++
UDB_RESULT udb_read_query_add_key(UDB_READ_QUERY read_query, char key, size_t key_size);
```

 Setting the labels in read_query.
 - `read_query`
 - `labels`
 - `label_count`

```c++
UDB_RESULT udb_read_query_set_labels(UDB_READ_QUERY read_query, char labels, size_t label_count);
```


 Getting the document using the read query. 
 - `conn`
 - `read_query`
 - `udb_document`
```c++
UDB_READ_QUERY_RESULTS udb_get(UDB_CONNECTION conn, UDB_READ_QUERY read_query);
```


 Get the ptr to the next result.
 - `results_container`
 - `result_ptr`
```c++
size_t udb_results_next(UDB_READ_QUERY_RESULTS results_container, UDB_READ_QUERY_RESULT result_ptr);
```

## READ QUERY GETTERS

```c++
size_t udb_get_results_count(UDB_READ_QUERY_RESULTS* results);
```
```c++
UDB_READ_QUERY_RESULT* udb_get_result(UDB_READ_QUERY_RESULTS* results, size_t index);
```