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

const char* get_sdk_version();

/**
* Opaque structure that holds the UltiDB state.
*
* Allocated and initialized with ::udb_create_instance.
* Cleaned up and deallocated with ::udb_destroy_instance.
*/
typedef struct UDB_STATE_STRUCT UDB;

/**
* Opaque structure that holds the config parameters in order to create an instance
 * of UltiDB.
*
* Allocated and initialized with ::udb_create_config.
* Cleaned up and deallocated with ::udb_destroy_config.
*/
typedef struct UDB_CONFIG_STRUCT UDB_CONFIG;

/**
 * Result given by the UDB APIs which tells if the operation was successful or not.
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

const char* get_error_message();

/** Chunking Strategy to choose from. When the data is large when integrating then it is automatically divided into
 * smaller pieces of data which we call chunks. This is done in order to increase the de-duplication ratio as smaller
 * data chunks lead to higher probability of de-duplication. If the type of data to be integrated is known beforehand
 * then a chunking strategy can be chosen accordingly. The chunking strategy to choose from are:
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

/*
 * Gives the error code for the last operation.
*/
UDB_RESULT udb_get_last_error();

// ---------------------------------------------------------------------

/**
* Here goes configuration options that are usually exposed via client command line.
*/
UDB_CONFIG* udb_create_config();

/**
 * Sets the connection parameter of the agency node in the given config file.
 *
 * @param cfg config file in which the connection parameters are set
 * @param hostname
 * @param port Port on which agency node is running
 * @param connection_pool The number of connections to create with the agency node.
 * @return bool true if the operation is successful, false if the operation failed
 */
UDB_RESULT udb_config_set_agencynode(UDB_CONFIG* cfg, const char *hostname, uint16_t port, size_t connection_pool = 3);

/**
 *
 * @param cfg config file in which the strategy is set
 * @param chunking_strategy The chunking strategy to apply
 * @return UDB_BOOL true if the operation is successful, false if the operation failed
 */
UDB_RESULT udb_config_set_chunking_strategy(UDB_CONFIG* cfg, UDB_CHUNKING_MODE chunking_strategy);

/**
 * Deallocates the intance created from ::udb_create_config.
 *
 * @param config config instance to be deallocated
 * @return UDB_RESULT result of the operation
 */
UDB_RESULT udb_destroy_config(UDB_CONFIG *config);

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
 * @param data Data to integrate into the UDB cluster
 * @param buffer_length
 * @param length Size of the data to integrate
 * @return unsigned int of 8 bits The value 0 means the operation was successful, else a integer value is set according
 * to the error encountered. This error can be deduced from enum ::UDB_RESULT.
 */
UDB_RESULT udb_integrate(UDB *db, char* hash_buffer,  size_t buffer_length, const char* data, size_t data_length);

/**
 * Data to retrieve from the UDB cluster based on the hash provided.
 *
 * @param db UDB object which can be used to perform operations such as integrate and retrieve.
 * @param data_buffer
 * @param length Size of the data retrieved
 * @return char* Returns nullptr if the operation is unsuccessful else a pointer to the underlying data is returned
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