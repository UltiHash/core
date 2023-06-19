/**
 * @file Exposes APIs in order to use UltiDB.
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
 * @return array of characters that holds the version of the SDK being used
 */
const char* get_sdk_version();

/**
* Opaque structure that holds the UDB state.
*
* Allocated and initialized with ::udb_create_instance.
* Cleaned up and deallocated with ::udb_destroy_instance.
*/
typedef struct UDB_STATE_STRUCT UDB;

/**
* Opaque structure that holds the configuration parameters to create an instance
 * of UDB.
*
* Allocated and initialized with ::udb_create_config.
* Cleaned up and deallocated with ::udb_destroy_config.
*/
typedef struct UDB_CONFIG_STRUCT UDB_CONFIG;

/**
 * ENUM given by UDB APIs which tells the result of the APIs called.
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

} UDB_RESULT;

/**
 * Gets the message of the last error occurred.
 */
const char* get_error_message();

/*
 * Gives the error code for the last operation.  The code is uint8_t integer.
*/
// is uint8_t enough?
UDB_RESULT udb_get_last_error();

/**
 * Structure that holds a unique key for a given document.
 */
typedef struct UDB_KEY_STRUCT
{
    char* key;
    size_t length;
} UDB_KEY;

/**
 * A UDB Document is a structure that holds a unique key and the associated data.
 */
typedef struct UDB_DOCUMENT_STRUCT
{
    char* data;
    size_t size;
} UDB_DOCUMENT;

/**
 * A struct that holds the underlying connection to the database.
 */
typedef struct UDB_CONNECTION_STRUCT UDB_CONNECTION;

// ---------------------------------------------------------------------

/**
 * Creates an instance of UDB_KEY structure and returns a pointer to it.
 *
 * @param key_buffer buffer which contains the unique key
 * @param length length of the key_buffer
 * @return UDB_KEY* pointer to the UDB_KEY struct created
 */
UDB_KEY* udb_create_key(char* key_buffer, size_t length);

/**
 * Deallocates the instance of UDB_KEY.
 *
 * @param udb_key
 * @return UDB_RESULT enum which describes the result of the operation
 */
UDB_RESULT udb_destroy_key(UDB_KEY* udb_key);

/**
 * Creates an instance of UDB_DOCUMENT structure and returns a pointer to it.
 *
 * @return UDB_DOCUMENT* pointer to the document structure allocated
 */
UDB_DOCUMENT* udb_create_document();

/**
 * Deallocates the instance of UDB_DOCUMENT which was allocated before. It also frees all the memory that is held
 * by the pointers inside the struct object.
 *
 * @param doc document to deallocate
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_destroy_document(UDB_DOCUMENT* doc);

/**
 * Creates an instance of UDB_DOCUMENT with a given key and returns a pointer to it.
 *
 * @return UDB_DOCUMENT* pointer to the document structure allocated
 */
UDB_RESULT udb_document_set_data(UDB_DOCUMENT* doc, char* data, size_t size);
UDB_RESULT udb_document_get_data(UDB_DOCUMENT* doc, char** data, size_t* size);

/**
* Creates an instance of ::UDB_CONFIG (typedef UDB_CONFIG) which can be used to put configuration parameters.
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
 *
 * @param handle UDB handle which can be used to perform various udb operations.
 * @param udb_docs array of UDB_DOCUMENT which are to be integrated
 * @param n_udb_docs number of items in the udb_docs array
 * @return UDB_RESULT enum which describes the result of the operation
 */
UDB_RESULT udb_add(UDB_CONNECTION* conn,
                   const UDB_DOCUMENT** docs,
                   UDB_KEY** key,
                   size_t n);

/**
 * Convenience function around `udb_insert`.
 *
 * @param handle UDB handle which can be used to perform various udb operations.
 * @param udb_docs a UDB_DOCUMENT which is to be integrated
 * @return UDB_RESULT enum which describes the result of the operation
 */
UDB_RESULT udb_add_one(UDB_CONNECTION* conn,
                       const UDB_DOCUMENT* doc,
                       UDB_KEY* key);

/**
 * Support our fast uploading without copy overhead.
 */
UDB_RESULT udb_add_continuous(UDB_CONNECTION* conn,
                              const char* buffer,
                              size_t size,
                              size_t* offsets,
                              size_t count,
                              UDB_KEY** out_key);

/**
 * See below for two variants of parameter passing.
 */
UDB_RESULT udb_get(UDB_CONNECTION* conn,
                   const UDB_KEY** key,
                   UDB_DOCUMENT** doc,
                   size_t n_key);

//UDB_RESULT udb_get_by_label(UDB* handle,
//                   const UDB_LABEL* label,
//                   size_t n_label,
//                   UDB_DOCUMENT** doc); //return documents containing all the labels

// Convenience
UDB_RESULT udb_get_one(UDB_CONNECTION* conn,
                       const UDB_KEY* key,
                       UDB_DOCUMENT* doc);

/**
 *
 * @param db UDB object which can be used to perform operations such as integrate and retrieve.
 * @param hash_buffer buffer in which the retrieved hash can be stored
 * @param buffer_length length of the hash_buffer provided
 * @param data pointer to the data to be integrated
 * @param length Size of the data to integrate
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_integrate(UDB_CONNECTION* udb_connection, char* hash_buffer,  size_t buffer_length, const char* data, size_t data_length);

/**
 * Data to retrieve from the UDB cluster based on the hash provided.
 *
 * @param db UDB object which can be used to perform operations such as integrate and retrieve.
 * @param data_buffer buffer in which data retrieved is to be stored
 * @param buffer_length length of the buffer provided
 * @param udb_hash the hash of the data to be retrieved
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_retrieve(UDB_CONNECTION* udb_connection, char* data_buffer, size_t buffer_length, const char* udb_hash);

// ---------------------------------------------------------------------

#if defined(__cplusplus) || defined(c_plusplus)
} // extern "C" {
#endif

#endif /* UDB_SDK_INCLUDE_UDB */

/* !!!!! Things to Improve
 * 1. Assumption of hash in retrieve being always 64 bytes. This is not very good.
 * 2. There is no chunking being done, and if we do, we do not get only one hash.
 * 3. pinging as of now is just checking the socket validity which is not ideal.
 */
