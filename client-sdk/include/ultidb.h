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

// ---------------------------------------------------------------------

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
 * @return UDB_RESULT enum that tells the result of the operation
 */
UDB_RESULT udb_config_set_host_node(UDB_CONFIG* cfg, const char *hostname, uint16_t port, size_t connection_pool = 3);

/**
 * Sets the chunking strategy in the given config file.
 *
 * @param cfg config file in which the strategy is set
 * @param chunking_strategy enum which describes the strategy to apply
 * @return UDB_RESULT enum that tells the result of the operation
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

/**
 *
 * @param db UDB object which can be used to perform operations such as integrate and retrieve.
 * @param hash_buffer buffer in which the retrieved hash can be stored
 * @param buffer_length length of the hash_buffer provided
 * @param data pointer to the data to be integrated
 * @param length Size of the data to integrate
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_integrate(UDB *db, char* hash_buffer,  size_t buffer_length, const char* data, size_t data_length);

/**
 * Data to retrieve from the UDB cluster based on the hash provided.
 *
 * @param db UDB object which can be used to perform operations such as integrate and retrieve.
 * @param data_buffer buffer in which data retrieved is to be stored
 * @param buffer_length length of the buffer provided
 * @param udb_hash the hash of the data to be retrieved
 * @return UDB_RESULT enum that describes the result of the operation
 */
UDB_RESULT udb_retrieve(UDB *db, char* data_buffer, size_t buffer_length, const char* udb_hash);

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
