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

// ---------------------------------------------------------------------

#define SDK_NAME "UDB"
#define SDK_VERSION "0.1.0"

/**
 *
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
 * A wrapper struct that wraps a char* and the size of the data the char* points to.
 *
 * It is used to wrap the keys and values that the user has.
 */
typedef struct UDB_DATA_WRAPPER
{
    char* data;
    size_t size;

    UDB_DATA_WRAPPER(char* rec_ptr, size_t rec_size) :
            data(rec_ptr), size(rec_size)
    {}

    UDB_DATA_WRAPPER() : data(nullptr), size(0) {}

} UDB_DATA;

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

// ---------------------------------------------------------------------

/**
* Creates an instance of ::UDB_CONFIG (typedef UDB_CONFIG) which can be used to
 * put configuration parameters.
* @return UDB_CONFIG* pointer to the config created
*/
UDB_CONFIG* udb_create_config();

/**
 * Deallocates the instance created from ::udb_create_config.
 *
 * @param config config instance to be deallocated
 * @return UDB_RESULT result of the operation
 */
UDB_RESULT udb_destroy_config(UDB_CONFIG *config);

/**
 * Sets the connection parameters of the host node in the given config file.
 *
 * @param cfg config file where the connection parameters can be set
 * @param hostname Name of the host to connect to
 * @param port Port on which the host is running
 * @param connection_pool The number of connections to create with the host. By default it is 3.
 * @return UDB_RESULT enum which describes the result of the operation
 */
UDB_RESULT udb_config_set_host_node(UDB_CONFIG* cfg, const char *hostname, uint16_t port, size_t connection_pool = 3);

/**
 * Creates an instance of ::UDB_STATE_STRUCT (typedef UDB) and initializes it.
 */
UDB* udb_create_instance(UDB_CONFIG *config);

/**
 * Destroys an instance of ::UDB_STATE_STRUCT (typedef UDB) and initializes it.
 *
 * @return UDB_RESULT result of the operation
 */
UDB_RESULT udb_destroy_instance(UDB* udb_instance);

// ---------------------------------------------------------------------

UDB_CONNECTION* udb_create_connection(UDB* instance);

UDB_RESULT udb_destroy_connection(UDB_CONNECTION* conn);

/**
 * Check if server (or connection) is still alive.
 *
 * @param handle UDB handle which can be used to perform various udb operations.
 * @return UDB_RESULT enum which describes the result of the operation
 */
UDB_RESULT udb_ping(UDB_CONNECTION* conn);

/**
 * Creates an instance of UDB_DOCUMENT structure and returns a pointer to it.
 *
 * @return UDB_DOCUMENT* pointer to the document structure allocated
 */
UDB_DOCUMENT* udb_init_document(UDB_DATA* key, UDB_DATA* value, char** labels, size_t label_count);
UDB_DOCUMENT* udb_create_document();

/**
 * Creates an instance of UDB_DOCUMENT with a given key and returns a pointer to it.
 *
 * @return UDB_DOCUMENT* pointer to the document structure allocated
 */
void udb_document_set_key(UDB_DOCUMENT* doc, UDB_DATA* key);
void udb_document_set_value(UDB_DOCUMENT* doc, UDB_DATA* value);
void udb_document_set_labels(UDB_DOCUMENT* doc, char** labels);
UDB_RESULT udb_document_add_label(UDB_DOCUMENT* doc, char* label);

/**
 * Deallocates the instance of UDB_DOCUMENT which was allocated before. It also frees all the memory that is held
 * by the pointers inside the struct object.
 *
 * @param doc document to deallocate
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_document(UDB_DOCUMENT** ptr_to_document_ptr);

UDB_WRITE_QUERY* udb_create_write_query();
void udb_write_query_add_document(UDB_WRITE_QUERY* write_query, UDB_DOCUMENT* document);
//UDB_RESULT udb_write_query_add_whole_dicument();
UDB_RESULT udb_destroy_write_query(UDB_WRITE_QUERY** write_query_ptr_container);

UDB_RESULT udb_add(UDB_CONNECTION* conn, UDB_WRITE_QUERY* write_query); // should also account for keys being empty in
// case user hasn't specified it

UDB_READ_QUERY* udb_create_read_query();
UDB_RESULT udb_read_query_add_key(UDB_READ_QUERY* read_query, UDB_DATA* key);
UDB_RESULT udb_read_query_set_key_range(UDB_READ_QUERY* read_query, UDB_DATA* start_key, UDB_DATA* end_key);
UDB_RESULT udb_read_query_set_labels(UDB_READ_QUERY* read_query, char** labels, size_t label_count);
UDB_RESULT udb_destroy_read_query(UDB_READ_QUERY** read_query_ptr_container);

UDB_RESULT udb_get(UDB_CONNECTION* conn, UDB_READ_QUERY* read_query, UDB_DOCUMENT** udb_document);

/* Getters */
size_t udb_get_documents_count(UDB_DOCUMENTS* docs);
UDB_DOCUMENT* udb_get_document(UDB_DOCUMENTS* docs, size_t index);
UDB_DATA* udb_get_key(UDB_DOCUMENT* doc);
UDB_DATA* udb_get_value(UDB_DOCUMENT* doc);
size_t udb_get_labels_count(UDB_DOCUMENT* doc);
char* udb_get_label(UDB_DOCUMENT* doc, size_t label_index);

// ---------------------------------------------------------------------

#if defined(__cplusplus) || defined(c_plusplus)
} // extern "C" {
#endif

#endif /* UDB_SDK_INCLUDE_UDB */
