/**
 * @file Exposes APIs in order to use UltiDB.
 */

#ifndef UDB_SDK_INCLUDE_UDB_H
#define UDB_SDK_INCLUDE_UDB_H

#include <cstddef>  /* for size_t */
#include <cstdint>

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

    /* Undefined chunking mode given. */
    UDB_UNDEFINED_CHUNKING_MODE,

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

/** Chunking Strategy to choose from. When the data is large then it is automatically divided into
 * smaller pieces of data which we call chunks. This is done in order to increase the de-duplication ratio as smaller
 * data chunks lead to higher probability of de-duplication. The chunking strategy to choose from are:
 * 1. Fixed Size
 * 2. Gear CDC
 * 3. Fast CDC
 * 4. Mod CDC
*/
typedef enum UDB_CHUNKING_MODE {

    /** */
    UDB_FIXED_SIZE_CHUNK = 0,

    /**  */
    UDB_GEAR_CDC = 1,

    /**  */
    UDB_FAST_CDC = 2,

    /** */
    UDB_MOD_CDC = 3,

    /** */
    UDB_RABIN_FINGERPRINT = 4,

} UDB_CHUNKING_MODE;

/**
 * Structure that holds a unique key for a given document.
 */
typedef struct UDB_KEY_STRUCT
{
    char* buffer;
    size_t length;
} UDB_KEY;

/**
 * A UDB Document is a structure that holds a unique key and the associated data.
 */
typedef struct UDB_DOCUMENT_STRUCT
{
    UDB_KEY* key;

    char* data;
    size_t size;
} UDB_DOCUMENT;

/**
 *
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
UDB_DOCUMENT* udb_create_document_with_buffer(char* key_buffer, size_t length);
UDB_DOCUMENT* udb_create_document_with_key(char* key_buffer, size_t length);

UDB_RESULT udb_document_set_data(UDB_DOCUMENT* doc, char* data, size_t size);
UDB_RESULT udb_document_get_data(UDB_DOCUMENT* doc, char** data, size_t* size);
UDB_RESULT udb_document_set_key(UDB_DOCUMENT* doc, UDB_KEY* key);
UDB_RESULT udb_document_get_key (UDB_DOCUMENT* doc, UDB_KEY** key);

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
 * Sets the chunking strategy in the given config file.
 *
 * @param cfg config file in which the strategy is set
 * @param chunking_strategy enum which describes the strategy to apply
 * @return UDB_RESULT enum which describes the result of the operation
 */
UDB_RESULT udb_config_set_chunking_strategy(UDB_CONFIG* cfg, UDB_CHUNKING_MODE chunking_strategy);

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

UDB_CONNECTION* udb_connect(UDB* handle);

/**
 * Check if server (or connection) is still alive.
 *
 * @param handle UDB handle which can be used to perform various udb operations.
 * @return UDB_RESULT enum which describes the result of the operation
 */
UDB_RESULT udb_ping(UDB_CONNECTION* udb_connection);

/**
 *
 * @param handle UDB handle which can be used to perform various udb operations.
 * @param udb_docs array of UDB_DOCUMENT which are to be integrated
 * @param n_udb_docs number of items in the udb_docs array
 * @return UDB_RESULT enum which describes the result of the operation
 */
UDB_RESULT udb_insert(UDB* handle,
                      const UDB_DOCUMENT** udb_docs,
                      size_t n_udb_docs);

/**
 * Insert and generate a key for the documents.
 */
UDB_RESULT udb_insert_and_genkey(UDB* handle,
                                 UDB_DOCUMENT** udb_docs,
                                 size_t n_udb_docs);

/**
 * Convenience function around `udb_insert`.
 *
 * @param handle UDB handle which can be used to perform various udb operations.
 * @param udb_docs a UDB_DOCUMENT which is to be integrated
 * @return UDB_RESULT enum which describes the result of the operation
 */
UDB_RESULT udb_insert_one(UDB* handle,
                          const UDB_DOCUMENT* doc);

/**
 * Convenience function around `udb_insert` which automatically generates a key for the document
 * given.
 *
 * @param handle UDB handle which can be used to perform various udb operations.
 * @param udb_docs a UDB_DOCUMENT which is to be integrated
 * @return UDB_RESULT enum which describes the result of the operation
 */
UDB_RESULT udb_insert_one_and_genkey(UDB* handle,
                                     const UDB_DOCUMENT* doc);

/**
 * Support our fast uploading without copy overhead.
 */
UDB_RESULT udb_insert_continuous_genkey(UDB* handle,
                                        const char* buffer,
                                        size_t size,
                                        size_t* offsets,
                                        size_t count,
                                        UDB_KEY** out_key);


/**
 * See below for two variants of parameter passing.
 */
UDB_RESULT udb_get(UDB* handle,
                   const UDB_KEY** key,
                   size_t n_key,
                   UDB_DOCUMENT** doc);

// Convenience
UDB_RESULT udb_get_one(UDB* handle,
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
 *
 */
