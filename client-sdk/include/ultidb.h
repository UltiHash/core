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
 * @return array of characters that gives the name of the SDK
 */
const char* get_sdk_name();

/**
 * @return array of characters that gives the version of the SDK
 */
const char* get_sdk_version();

/**
 * Gets the message of the last error occurred.
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
 *
 * @return enum of type ::UDB_RESULT which describes the last error occurred.
 */
UDB_RESULT udb_get_last_error();

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
 * Opaque structure that holds the underlying connection to the database.
 */
typedef struct UDB_CONNECTION_STRUCT UDB_CONNECTION;

/**
 * Opaque structure that represents the concept of a "document". A document is a
 * structure that holds a key, value, and labels.
 */
typedef struct UDB_DOCUMENT_STRUCT UDB_DOCUMENT;

/**
 * Opaque structure that describes a write query for writing documents into the database.
 *
 * Provided functions can be used to fill up this query structure and subsequently write
 * documents to the database.
 */
typedef struct UDB_DOCUMENTS UDB_WRITE_QUERY;

/**
 * Opaque structure that describes a read query for reading documents from the database.
 *
 * Provided functions can be used to fill up this query structure and subsequently read
 * documents from the database.
 */
typedef struct UDB_READ_QUERY_STRUCT UDB_READ_QUERY;

/**
 * A wrapper struct that wraps a char* and the size of the pointed data.
 *
 * It is used to wrap the keys and values for convenience purposes.
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
 * A structure that holds key, value and labels.
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
 * A opaque structure that holds
 */
struct UDB_READ_QUERY_RESULTS;

/**
 * Creates an instance of ::UDB_CONFIG which can be used to put configuration parameters.
 *
 * The returned pointer to the ::UDB_CONFIG structure has to be deallocated using
 * ::udb_destroy_config.
 *
 * @return pointer to the ::UDB_CONFIG structure created.
 */
UDB_CONFIG* udb_create_config();

/**
 * Deallocates the instance created from ::udb_create_config.
 *
 * @param config config instance to be deallocated
 * @return enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_config(UDB_CONFIG* config);

/**
 * Sets the connection parameters of the host node in the given config file.
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
 * @return pointer to the ::UDB structure created
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
 * Creates an instance of ::UDB_CONNECTION which can be used to perform various
 * UDB operations such as adding and getting documents to/from the UDB database.
 *
 * The instance created has to be destroyed using ::udb_destroy_connection.
 *
 * @param instance ::UDB instance to use in order to create a connection to the database
 * @return pointer to the ::UDB_CONNECTION instance
 */
UDB_CONNECTION* udb_create_connection(UDB* instance);

/**
 * Destroys an instance of ::UDB_CONNECTION created via ::udb_create_connection.
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
 * Creates an instance of ::UDB_DOCUMENT structure and returns a pointer it
 * if the allocation was successful. On failure, a nullptr is returned.
 *
 * @param key pointer to the array of key characters
 * @param key_size size of the key
 * @param value pointer to the array of value characters
 * @param value_size size of the value
 * @param labels char** which holds pointers to the labels. The labels themselves are
 * null terminated.
 * @param label_count Number of labels the user wants to assign for the value
 * @return pointer to the ::UDB_DOCUMENT allocated. On failure a nullptr is returned.
*/
UDB_DOCUMENT* udb_init_document(char* key, size_t key_size, char* value, size_t value_size,
                                char** labels, size_t label_count);

/**
 * Deallocates the instance of ::UDB_DOCUMENT structure. If the document instance was set
 * to have an owning enum through ::udb, it recursively deallocates references to all the
 * object it holds, else it only deallocates itself.
 *
 * @param doc document to deallocate
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_document(UDB_DOCUMENT* ptr_to_document_ptr);

/**
 * Creating a write query to use when putting a document.
 */
UDB_WRITE_QUERY* udb_create_write_query();

/**
 * Adding a document in the write query.
 * @param write_query The ::UDB_WRITE_QUERY struct in which a document is to be added
 * @param document ::UDB_DOCUMENT struct to add
 */
void udb_write_query_add_document(UDB_WRITE_QUERY* write_query, UDB_DOCUMENT* document);

/**
 * Destroying the instance of write query.
 *
 * @param write_query pointer to the variable that contains the pointer to the previously
 * allocated ::UDB_WRITE_QUERY. This is done so that the nullptr can be set after de-allocation
 * on the variable as the pointer contained is no longer valid
 *
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_write_query(UDB_WRITE_QUERY* write_query);

/**
 * pointer to the results given by udb_add.
 */
struct UDB_WRITE_QUERY_RESULTS;

/**
 * Destroying the instance of ::UDB_WRITE_QUERY_RESULTS.
 *
 * @param results pointer to the variable that contains the pointer to the previously allocated
 * ::UDB_WRITE_QUERY_RESULTS. This is done so that the nullptr can be set after de-allocation
 * on the variable as the pointer contained is no longer valid.

 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_write_query_results(UDB_WRITE_QUERY_RESULTS* results);

/**
 * Gets the count of the effective sizes returned after putting each document from the write
 * query in the database.
 * @param results pointer to the ::UDB_WRITE_QUERY_RESULTS struct
 * @return size_t count of the effective sizes returned for each document put in the database
 */
size_t udb_get_effective_sizes_count(UDB_WRITE_QUERY_RESULTS* results);

/**
 * Gets the effective size of the index given from the ::UDB_WRITE_QUERY_RESULTS struct.
 * The effective sizes are in same order as the documents inserted in the ::UDB_WRITE_QUERY
 *
 * @param results pointer to the ::UDB_WRITE_QUERY_RESULTS struct
 * @param index gives the index from which to get the effective size from
 * @return uint32_t effective size of the given index from the ::UDB_WRITE_QUERY_RESULTS
 */
uint32_t udb_get_effective_size(UDB_WRITE_QUERY_RESULTS* results, size_t index);

/**
 * Putting the document in the database.
 * @param conn pointer to ::UDB_CONNECTION struct
 * @param write_query pointer to the ::UDB_WRITE_QUERY which contains documents to be put
 * in the database
 *
 * @return UDB_WRITE_QUERY_RESULTS* pointer to the ::UDB_WRITE_QUERY_RESULTS which contains
 * the result of the operation such as the effective size used by each document in the database
 */
UDB_WRITE_QUERY_RESULTS* udb_put(UDB_CONNECTION* conn, UDB_WRITE_QUERY* write_query);

/**
 * Allocates the UDB_READ_QUERY struct for constructing read query for getting documents from
 * the database.
 *
 * @return UDB_READ_QUERY* pointer to the ::UDB_READ_QUERY struct allocated.
 */
UDB_READ_QUERY* udb_create_read_query();

/**
 * Adding keys to the read_query.
 *
 * @param read_query pointer to ::UDB_READ_QUERY struct in which to add the given key
 * @param key pointer to the array of key characters
 * @param key_size size of the key given
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_read_query_add_key(UDB_READ_QUERY* read_query, char* key, size_t key_size);

/**
 * Setting the labels in read_query.
 * @param read_query pointer to ::UDB_READ_QUERY struct in which to add the given labels
 * @param labels pointer to an array containing pointers to null terminated null label strings
 * @param label_count number of label strings given
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_read_query_set_labels(UDB_READ_QUERY* read_query, char** labels, size_t label_count);

/**
 * Destroys the allocated ::UDB_READ_QUERY struct.
 *
 * @param read_query_ptr_container pointer to a variable containing the pointer to the allocated
 * ::UDB_READ_QUERY struct. This is done such that variable can be set to nullptr as the pointer to
 * the ::UDB_READ_QUERY struct is no longer valid.
 *
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_read_query(UDB_READ_QUERY* read_query_ptr_container);

/**
 * Getting the document using the ::UDB_READ_QUERY struct. Returns a pointer to the ::UDB_READ_QUERY_RESULTS
 * struct which contains results of the operation. ::udb_results_next function can be used to go through the
 * returned ::UDB_READ_QUERY_RESULTS.
 *
 * @param conn pointer to ::UDB_CONNECTION struct
 * @param read_query pointer to the read query used to get the document from the database
 * @return UDB_READ_QUERY_RESULTS* pointer to the struct that contains the results of the get operation.
 * The struct contains all the retrieved documents.
 */
UDB_READ_QUERY_RESULTS* udb_get(UDB_CONNECTION* conn, UDB_READ_QUERY* read_query);

/**
 * Gets the pointer to the next ::UDB_READ_QUERY_RESULT from the ::UDB_READ_QUERY_RESULTS struct.
 *
 * @param results_container pointer to the UDB_READ_QUERY_RESULTS struct which contains the UDB_READ_QUERY_RESULT
 * @param result_ptr pointer to the variable which will hold the retrieved pointer to UDB_READ_QUERY_RESULT from
 * UDB_READ_QUERY_RESULTS
 * @return returns true if there is a UDB_READ_QUERY_RESULT in the UDB_READ_QUERY_RESULTS else it returns false.
 * On subsequent call to ::udb_results_next, the function will start retrieving the pointer from
 * the beginning.
 *
 */
bool udb_results_next(UDB_READ_QUERY_RESULTS* results_container, UDB_READ_QUERY_RESULT** result_ptr);

/**
 * Deallocates the previously allocated ::UDB_READ_QUERY_RESULTS struct.
 *
 * @param results pointer to a variable containing the pointer to the allocated
 * ::UDB_READ_QUERY_RESULTS struct. This is done such that variable can be set to nullptr as the pointer to
 * the ::UDB_READ_QUERY_RESULTS struct is no longer valid.
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
 * @return pointer to the ::UDB_READ_QUERY_RESULT struct
 */
UDB_READ_QUERY_RESULT* udb_get_result(UDB_READ_QUERY_RESULTS* results, size_t index);

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
 * @param doc pointer to the ::UDB_DOCUMENT struct
 * @return pointer to the UDB_DATA struct which holds the key information i.e. char* and size_t
 */
UDB_DATA* udb_get_key(UDB_DOCUMENT* doc);

/**
 * Gets the value of the document. Returns a pointer to ::UDB_DATA which holds the value
 * information i.e. char* and size_t
 *
 * @param doc pointer to the ::UDB_DOCUMENT struct
 * @return pointer to the UDB_DATA struct which holds the key information i.e. char* and size_t
 */
UDB_DATA* udb_get_value(UDB_DOCUMENT* doc);

/**
 * Gets the count of the labels inside the document.
 *
 * @param doc pointer to the ::UDB_DOCUMENT struct
 * @return number of labels that is in the document
 */
size_t udb_get_labels_count(UDB_DOCUMENT* doc);

/**
 * Gets the pointer to the null-terminated label string from the given index.
 *
 * @param doc pointer to the document
 * @param label_index index of the label string which is to be retrieved
 * @return pointer to the null-terminated label string at the given index
 */
char* udb_get_label(UDB_DOCUMENT* doc, size_t label_index);

// ---------------------------------------------------------------------

#if defined(__cplusplus) || defined(c_plusplus)
} // extern "C" {
#endif

#endif /* UDB_SDK_INCLUDE_UDB */
