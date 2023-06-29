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

} UDB_RESULT;

/**
 *
 * @return enum of type ::UDB_RESULT which describes the last error occurred.
 */
UDB_RESULT udb_get_last_error();

/**
 * Instructs the ::UDB_DOCUMENT whether to deallocate the underlying memory it holds
 * or not.
 *
 * A document holds pointer to 3 major things: 1. UDB_DATA* key, 2. UDB_DATA* value,
 * 3. char** labels. ::OWNING_TYPE can be set to owning in the ::UDB_DOCUMENT instance
 * to instruct that the document has full ownership of the objects being referenced by
 * these pointers. As a result when ::UDB_DOCUMENT instance is being destroyed, it will
 * also destroy the underlying objects.
 */
typedef enum : uint8_t
{
    non_owning = 0,
    owning
} OWNING_TYPE;

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
                          key(rec_key), key_size(rec_value_size), value(rec_value), value_size(rec_value_size),
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
UDB_RESULT udb_destroy_config(UDB_CONFIG *config);

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
 * @param key UDB_DATA* which holds the key information
 * @param value UDB_DATA* which holds the value information
 * @param labels char** which holds pointers to the labels. The labels themselves are null terminated.
 * @param label_count Number of labels the user wants to assign for the value
 * @return pointer to the ::UDB_DOCUMENT allocated. On failure a nullptr is returned.
*/
UDB_DOCUMENT* udb_init_document(char* key, size_t key_size, char* value, size_t value_size,
                                char** labels, size_t label_count);

/**
 * Deallocates the instance of ::UDB_DOCUMENT structure. If the document instance was set to have an
 * owning enum through ::udb, it recursively deallocates references to all the object it holds, else it only
 * deallocates itself.
 *
 * @param doc document to deallocate
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_document(UDB_DOCUMENT** ptr_to_document_ptr);

/**
 * Creating a write query to use when putting a document.
 */
UDB_WRITE_QUERY* udb_create_write_query();

/**
 * Adding a document in the write query.
 * @param write_query
 * @param document
 */
void udb_write_query_add_document(UDB_WRITE_QUERY* write_query, UDB_DOCUMENT* document);

/**
 * Destorying the instance of write query.
 */
UDB_RESULT udb_destroy_write_query(UDB_WRITE_QUERY** write_query_ptr_container);

/**
 * pointer to the results given by udb_add.
 */
struct UDB_WRITE_QUERY_RESULTS;

UDB_RESULT udb_destroy_write_query_results(UDB_WRITE_QUERY_RESULTS** results);

size_t udb_get_effective_sizes_count(UDB_WRITE_QUERY_RESULTS* results);
uint32_t udb_get_effective_size(UDB_WRITE_QUERY_RESULTS* results, size_t index);

/**
 * Putting the document in the database.
 * @param conn
 * @param write_query
 * @return
 */
UDB_WRITE_QUERY_RESULTS* udb_put(UDB_CONNECTION* conn, UDB_WRITE_QUERY* write_query);

/**
 * Creating a read query for reading documents.
 * @return
 */
UDB_READ_QUERY* udb_create_read_query();

/**
 * Adding keys to the read_query.
 * @param read_query
 * @param key
 * @param key_size
 * @return
 */
UDB_RESULT udb_read_query_add_key(UDB_READ_QUERY* read_query, char* key, size_t key_size);

/**
 * Setting the labels in read_query.
 * @param read_query
 * @param labels
 * @param label_count
 * @return
 */
UDB_RESULT udb_read_query_set_labels(UDB_READ_QUERY* read_query, char** labels, size_t label_count);

/**
 * Destroying a read_query.
 * @param read_query_ptr_container
 * @return
 */
UDB_RESULT udb_destroy_read_query(UDB_READ_QUERY** read_query_ptr_container);

/**
 * Getting the document using the read query. Returns a
 * @param conn
 * @param read_query
 * @param udb_document
 * @return
 */
UDB_READ_QUERY_RESULTS* udb_get(UDB_CONNECTION* conn, UDB_READ_QUERY* read_query);

/**
 * Get the ptr to the next result.
 * @param results_container
 * @param result_ptr
 * @return
 */
bool udb_results_next(UDB_READ_QUERY_RESULTS* results_container, UDB_READ_QUERY_RESULT** result_ptr);

UDB_RESULT udb_destroy_read_query_results(UDB_READ_QUERY_RESULTS** results);

/* Getters */
size_t udb_get_results_count(UDB_READ_QUERY_RESULTS* results);
UDB_READ_QUERY_RESULT* udb_get_result(UDB_READ_QUERY_RESULTS* results, size_t index);
UDB_DATA* udb_get_key(UDB_DOCUMENT* doc);
UDB_DATA* udb_get_value(UDB_DOCUMENT* doc);
size_t udb_get_labels_count(UDB_DOCUMENT* doc);
char* udb_get_label(UDB_DOCUMENT* doc, size_t label_index);

// ---------------------------------------------------------------------

#if defined(__cplusplus) || defined(c_plusplus)
} // extern "C" {
#endif

#endif /* UDB_SDK_INCLUDE_UDB */
