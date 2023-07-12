/**
 * @file Exposes APIs for UDB.
 */

#ifndef UDB_SDK_INCLUDE_UDB_H
#define UDB_SDK_INCLUDE_UDB_H

#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define SDK_NAME "UDB"
#define SDK_VERSION "0.1.0"

// ---------------------------------------------------------------------


/**
 * Gets the name of the SDK being used.
 *
 * @return const char* that points to the array containing the name of the sdk
 */
const char* get_sdk_name();

/**
 * Gets the version of the SDK being used.
 *
 * @return const char* that points to the array containing the version of the sdk
 */
const char* get_sdk_version();

/**
 * RESULTS AND ERRORS
*/

/**
 * Gets the message of the last error occurred.
 *
 * @return const char* that points to the array containing the last occurred error message
 */
const char* get_error_message();

/**
 * ENUM values that describes the result of API calls.
 */
typedef enum : uint8_t
{
    /* UDB operation successfully completed. */
    UDB_RESULT_SUCCESS = 0,

    /* UDB operation failed and the error is not known. */
    UDB_RESULT_ERROR,

    /* Data couldn't be fit in the buffer given. */
    UDB_BUFFER_OVERFLOW,

    /* Memory couldn't be allocated. */
    UDB_BAD_ALLOCATION,

    /* Cannot set key(s) as a key type has already been previously set. */
    UDB_KEY_ALREADY_SET,

    /* Key was not set when using ::UDB_READ_QUERY. */
    UDB_UNINITIALIZED_KEY,

    /* Server was not found. */
    UDB_SERVER_CONNECTION_ERROR,

} UDB_RESULT;

/**
 * Gets the enum of type ::UDB_RESULT from the last error occurred.
 *
 * @return enum of type ::UDB_RESULT which describes the last error occurred.
 */
UDB_RESULT udb_get_last_error();


/**
 * CONFIGURATION
*/

/**
* Opaque structure that holds the underlying UDB instance.
*
* Allocated and initialized with ::udb_create_instance.
* Cleaned up and deallocated with ::udb_destroy_instance.
*/
typedef struct UDB_STATE_STRUCT UDB;

/**
* Opaque structure that holds the configuration parameters to create an instance of
 * ::UDB.
*
* Allocated and initialized with ::udb_create_config.
* Cleaned up and deallocated with ::udb_destroy_config.
*/
typedef struct UDB_CONFIG_STRUCT UDB_CONFIG;

/**
 * Creates an instance of ::UDB_CONFIG which can be used to put configuration parameters.
 *
 * The returned pointer to the ::UDB_CONFIG structure has to be deallocated using
 * ::udb_destroy_config as it is allocated on ::udb_create_config function call.
 *
 * @return pointer to the ::UDB_CONFIG structure created. On error nullptr is returned.
 */
UDB_CONFIG* udb_create_config();

/**
 * Deallocates the instance of ::UDB_CONFIG created from ::udb_create_config.
 *
 * @param config ::UDB_CONFIG instance to be deallocated
 * @return enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_config(UDB_CONFIG* config);

/**
 * Sets the connection parameters of the host node in the given ::UDB_CONFIG instance.
 *
 * @param cfg config structure where the connection parameters can be set
 * @param hostname Name of the host to connect to
 * @param port Port on which the host is running
 * @return enum that describes the result of the operation
 */
UDB_RESULT udb_config_set_host_node(UDB_CONFIG* cfg, const char *hostname, uint16_t port);

/**
 * Creates an instance of ::UDB with the given ::UDB_CONFIG instance.
 *
 * The instance of the ::UDB created has to be deallocated using ::udb_destroy_instance.
 *
 * @param config ::UDB_CONFIG instance where the configuration parameters are set
 * @return pointer to the ::UDB structure created. On error nullptr is returned.
 */
UDB* udb_create_instance(UDB_CONFIG *config);

/**
 * Destroys an instance of ::UDB created via ::udb_create_instance.
 *
 * @param udb_instance ::UDB instance to destroy
 * @return enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_instance(UDB* udb_instance);


/**
 *  STARTUP
*/

/**
 * Opaque structure that holds the underlying connection to the database.
 *
 * Allocated and initialized with ::udb_create_connection.
 * Cleaned up and deallocated with ::udb_destroy_connection.
 */
typedef struct UDB_CONNECTION_STRUCT UDB_CONNECTION;

/**
 * Creates an instance of ::UDB_CONNECTION which can be used to perform various
 * UDB operations such as adding and getting objects to/from the UDB database.
 *
 * The instance created has to be destroyed using ::udb_destroy_connection.
 *
 * @param instance ::UDB instance to use in order to create a connection to the database
 * @return pointer to the ::UDB_CONNECTION instance. On error nullptr is returned.
 */
UDB_CONNECTION* udb_create_connection(UDB* instance);

/**
 * Destroys an instance of ::UDB_CONNECTION created via ::udb_create_connection.
 *
 * @param conn ::UDB_CONNECTION instance to deallocate
 * @return enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_connection(UDB_CONNECTION* conn);

/**
 * Check if database connection is still alive.
 *
 * @param conn UDB connection which can be used to perform various UDB operations.
 * @return enum which describes the result of the operation
 */
UDB_RESULT udb_ping(UDB_CONNECTION* conn);

/**
 *  QUERY
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
 * Opaque structure that describes a write query for writing objects into the database.
 *
 * Provided functions can be used to fill up this query structure and subsequently write
 * objects to the database.
 *
 * Allocated and initialized with ::udb_create_write_query.
 * Cleaned up and deallocated with ::udb_destroy_write_query.
 */
typedef struct UDB_OBJECTS UDB_WRITE_QUERY;

/**
 * Opaque structure that describes a read query for reading objects from the database.
 *
 * Provided functions can be used to fill up this query structure and subsequently read
 * objects from the database.
 *
 * Allocated and initialized with ::udb_create_read_query.
 * Cleaned up and deallocated with ::udb_destroy_read_query.
 */
typedef struct UDB_READ_QUERY_STRUCT UDB_READ_QUERY;


/**
 * A structure that holds key, value and labels information.
 *
 * A ::UDB_READ_QUERY_RESULT struct is retrieved from ::UDB_READ_QUERY_RESULTS struct by using the functions
 * provided. When the ::udb_get function is called with appropriate arguments, a pointer to the
 * ::UDB_READ_QUERY_RESULTS struct is returned which contains an array of ::UDB_READ_QUERY_RESULT describing the
 * individual result retrieved from the read query.
 */
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

/**
 * An opaque structure that holds all the results retrieved from the ::udb_get function.
 *
 * Cleaned up and deallocated with ::udb_destroy_read_query_results.
 */
struct UDB_READ_QUERY_RESULTS;

/**
 * Creates a write query for putting an object into the database.
 *
 * The allocated ::UDB_WRITE_QUERY has to be deallocated using ::udb_destroy_write_query.
 *
 * @return pointer to the ::UDB_WRITE_QUERY created. On error nullptr is returned.
 */
UDB_WRITE_QUERY* udb_create_write_query();

/**
 * Adds an object in the write query.
 *
 * @param write_query The ::UDB_WRITE_QUERY struct in which an object is to be added
 * @param object ::UDB_OBJECT struct to add
 */
void udb_write_query_add_object(UDB_WRITE_QUERY* write_query, UDB_OBJECT* object);

/**
 * Destroying the instance of write query.
 *
 * @param write_query pointer to the allocated ::UDB_WRITE_QUERY
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_write_query(UDB_WRITE_QUERY* write_query);

/**
 * An opaque struct which holds the results received from ::udb_put function call.
 *
 * Cleaned up and deallocated with ::udb_destroy_write_query_results.
 */
struct UDB_WRITE_QUERY_RESULTS;

/**
 * Destroying the instance of ::UDB_WRITE_QUERY_RESULTS.
 *
 * @param results pointer to the ::UDB_WRITE_QUERY_RESULTS
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_write_query_results(UDB_WRITE_QUERY_RESULTS* results);

/**
 * Gets the number of the effective sizes returned in the ::UDB_WRITE_QUERY_RESULTS. Each object
 * put in the database has an effective size returned.
 *
 * @param results pointer to the ::UDB_WRITE_QUERY_RESULTS struct
 * @return size_t count of the effective sizes returned for each object put in the database
 */
size_t udb_get_effective_sizes_count(UDB_WRITE_QUERY_RESULTS* results);

/**
 * Gets the effective size from the index given inside the ::UDB_WRITE_QUERY_RESULTS struct.
 * The effective sizes are in same order as the objects inserted in the ::UDB_WRITE_QUERY.
 *
 * @param results pointer to the ::UDB_WRITE_QUERY_RESULTS struct
 * @param index gives the index from which to get the effective size from
 * @return uint32_t effective size of the given index from the ::UDB_WRITE_QUERY_RESULTS
 */
uint32_t udb_get_effective_size(UDB_WRITE_QUERY_RESULTS* results, size_t index);

/**
 * Gets the count of the return code returned in the ::UDB_WRITE_QUERY_RESULTS struct. ::UDB_WRITE_QUERY_RESULTS
 * struct stores an array of return codes for each objects in the write query.
 *
 * @param results pointer to the ::UDB_WRITE_QUERY_RESULTS struct
 * @return count of the return codes returned in ::UDB_WRITE_QUERY_RESULTS struct
 */
size_t udb_get_return_code_count(UDB_WRITE_QUERY_RESULTS* results);

/**
 * Gets the return code at the given index. The order of return codes returned is the same as the order of the
 * objects added in the write query. ::UDB_WRITE_QUERY_RESULTS struct stores an array of return codes for each
 * objects in the write query.
 *
 * @param results pointer to the ::UDB_WRITE_QUERY_RESULTS struct
 * @param index index from which return code is to be received
 * @return return code of each objects `put` into the database
 */
uint8_t udb_get_return_code(UDB_WRITE_QUERY_RESULTS* results, size_t index);

/**
 * Puts the object in the database.
 *
 * @param conn pointer to ::UDB_CONNECTION struct
 * @param write_query pointer to the ::UDB_WRITE_QUERY struct which contains the objects to be put
 * in the database
 * @return pointer to the ::UDB_WRITE_QUERY_RESULTS which contains the results such as
 * the effective sizes and the return code of each object. On error nullptr is returned.
 */
UDB_WRITE_QUERY_RESULTS* udb_put(UDB_CONNECTION* conn, UDB_WRITE_QUERY* write_query);

/**
 * Allocates the ::UDB_READ_QUERY struct for constructing read query in order to get objects from
 * the database.
 *
 * The allocated instance has to be deallocated with ::udb_destroy_read_query.
 *
 * @return UDB_READ_QUERY* pointer to the ::UDB_READ_QUERY struct allocated. On error nullptr is returned.
 */
UDB_READ_QUERY* udb_create_read_query();

/**
 * Adds keys to the read_query.
 *
 * @param read_query pointer to ::UDB_READ_QUERY struct in which the given key is to be added
 * @param key pointer to the array of key characters
 * @param key_size size of the key given
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_read_query_add_key(UDB_READ_QUERY* read_query, char* key, size_t key_size);

/**
 * Setting the labels in read_query.
 *
 * @param read_query pointer to ::UDB_READ_QUERY struct in which to add the given labels
 * @param labels pointer to an array containing pointers to null terminated null label strings
 * @param label_count number of label strings given
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_read_query_set_labels(UDB_READ_QUERY* read_query, char** labels, size_t label_count);

/**
 * Destroys the allocated ::UDB_READ_QUERY struct.
 *
 * @param read_query pointer to the allocated ::UDB_READ_QUERY struct
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_read_query(UDB_READ_QUERY* read_query);

/**
 * Getting the object using the ::UDB_READ_QUERY struct. Returns a pointer to the ::UDB_READ_QUERY_RESULTS
 * struct which contains results of the ::udb_get operation. ::udb_results_next function can be used to go through the
 * returned ::UDB_READ_QUERY_RESULTS struct.
 *
 * @param conn pointer to ::UDB_CONNECTION struct
 * @param read_query pointer to the read query used to get the object from the database
 * @return UDB_READ_QUERY_RESULTS* pointer to the struct that contains the results of the ::udb_get operation.
 * The struct contains all the retrieved objects. On error nullptr is returned.
 */
UDB_READ_QUERY_RESULTS* udb_get(UDB_CONNECTION* conn, UDB_READ_QUERY* read_query);

/**
 * Gets the pointer to the next ::UDB_READ_QUERY_RESULT from the ::UDB_READ_QUERY_RESULTS struct.
 *
 * @param results_container pointer to the ::UDB_READ_QUERY_RESULTS struct which contains the array of
 * ::UDB_READ_QUERY_RESULT struct
 * @param result_ptr pointer to the variable which will hold the retrieved pointer to ::UDB_READ_QUERY_RESULT
 * @return returns true if there is the next ::UDB_READ_QUERY_RESULT in the ::UDB_READ_QUERY_RESULTS struct
 * else it returns false. On subsequent call to ::udb_results_next, the function will start retrieving the pointer from
 * the beginning.
 *
 */
bool udb_results_next(UDB_READ_QUERY_RESULTS* results_container, UDB_READ_QUERY_RESULT** result_ptr);

/**
 * Deallocates the previously allocated ::UDB_READ_QUERY_RESULTS struct.
 *
 * @param results pointer to the allocated ::UDB_READ_QUERY_RESULTS struct
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_read_query_results(UDB_READ_QUERY_RESULTS* results);

/**
 * Gets the count of ::UDB_READ_QUERY_RESULT inside the ::UDB_READ_QUERY_RESULTS struct
 *
 * @param results pointer to the ::UDB_READ_QUERY_RESULTS struct
 * @return size_t number of ::UDB_READ_QUERY_RESULT in the retrieved read query
 */
size_t udb_get_results_count(UDB_READ_QUERY_RESULTS* results);

/**
 * Gets the pointer to the ::UDB_READ_QUERY_RESULT at a given index inside the ::UDB_READ_QUERY_RESULTS struct
 * @param results pointer to the ::UDB_READ_QUERY_RESULTS struct
 * @param index position of ::UDB_READ_QUERY_RESULT inside ::UDB_READ_QUERY_RESULTS to get
 * @return pointer to the ::UDB_READ_QUERY_RESULT struct. On error nullptr is returned.
 */
UDB_READ_QUERY_RESULT* udb_get_result(UDB_READ_QUERY_RESULTS* results, size_t index);


/**
 * OBJECTS
*/

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

// ---------------------------------------------------------------------

#if defined(__cplusplus) || defined(c_plusplus)
} // extern "C" {
#endif

#endif /* UDB_SDK_INCLUDE_UDB */
