# C API - Documents


/**
 * OBJECTS
*/

/**
 * Opaque structure that represents the concept of an "object". An object is a
 * structure that holds a key, value, and labels.
 *
 * Allocated and initialized with ::udb_init_object.
 * Cleaned up and deallocated with ::udb_destroy_object.
 */
typedef struct UDB_OBJECT_STRUCT UDB_OBJECT;

/**
 * A wrapper struct that wraps a char* and the size of the pointed data.
 *
 * It is used to wrap the keys and values of the objects when `getting` from objects for convenience purposes.
 */
struct UDB_DATA
{
    char* data;
    size_t size;

    UDB_DATA(char* rec_ptr, size_t rec_size) :
            data(rec_ptr), size(rec_size)
    {}

    UDB_DATA() : data(nullptr), size(0) {}

};

## Create
```c++
/**
 * Creates an instance of ::UDB_OBJECT structure.
 *
 * The allocated instance of ::UDB_OBJECT has to be deallocated with ::udb_destroy_object.
 *
 * @param key pointer to the array of key characters
 * @param key_size size of the key
 * @param value pointer to the array of value characters
 * @param value_size size of the value
 * @param labels char** which holds pointers to the labels. The label strings themselves are
 * null terminated.
 * @param label_count Number of labels the user wants to assign for the value
 * @return pointer to the ::UDB_OBJECT allocated. On error nullptr is returned.
*/
UDB_OBJECT* udb_init_object(char* key, size_t key_size, char* value, size_t value_size,
                                char** labels, size_t label_count);

```

## Destroy

```c++

/**
 * Deallocates the instance of ::UDB_OBJECT structure created via ::udb_init_object.
 *
 * @param obj object to deallocate
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_object(UDB_OBJECT* obj);


/**
 * Deallocates the ::UDB_DATA struct which was allocated before.
 *
 * @param data pointer to the ::UDB_DATA struct
 * @return enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_udb_data(UDB_DATA* data);
```

## Getters

```c++
/**
 * Gets a pointer to the ::UDB_DATA struct which holds the key information. The struct is allocated and has to
 * be deallocated with ::udb_destroy_udb_data function.
 *
 * @param obj pointer to the ::UDB_OBJECT struct
 * @return pointer to the UDB_DATA struct which holds the key information i.e. char* and size_t.
 * On error nullptr is returned.
 */
UDB_DATA* udb_get_key(UDB_OBJECT* obj);

/**
 * Gets the value of the object. Returns a pointer to ::UDB_DATA which holds the value
 * information i.e. char* and size_t
 *
 * @param obj pointer to the ::UDB_OBJECT struct
 * @return pointer to the UDB_DATA struct which holds the key information i.e. char* and size_t.
 * On error nullptr is returned.
 */
UDB_DATA* udb_get_value(UDB_OBJECT* obj);

/**
 * Gets the count of the labels inside the object.
 *
 * @param obj pointer to the ::UDB_OBJECT struct
 * @return number of labels that is in the object
 */
size_t udb_get_labels_count(UDB_OBJECT* obj);

/**
 * Gets the pointer to the null-terminated label string from the given index.
 *
 * @param obj pointer to the object
 * @param label_index index of the label string which is to be retrieved
 * @return pointer to the null-terminated label string at the given index.
 * On error nullptr is returned.
 */
char* udb_get_label(UDB_OBJECT* obj, size_t label_index);

```