/**
 * @file Exposes APIs to use UltiDB.
 */

#ifndef ULTIDB_SDK_INCLUDE_ULTIDB_H
#define ULTIDB_SDK_INCLUDE_ULTIDB_H

#include "types.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

// ---------------------------------------------------------------------

/**
* Opaque structure that holds the UltiDB state.
*
* Allocated and initialized with ::udb_create_instance.
* Cleaned up and deallocated with ::udb_destroy_instance.
*/
typedef struct ULTIDB_STATE_STRUCT ULTIDB;

/**
* Opaque structure that holds the config parameters in order to create an instance
 * of UltiDB.
*
* Allocated and initialized with ::UltiDBCreateInstance.
* Cleaned up and deallocated with ::UltiDBDestroyInstance.
*/
typedef struct ULTIDB_CONFIG_STRUCT ULTIDB_CONFIG;

/**
 * Result type for ULTIDB APIs.
 */
typedef enum : uint8_t
{
    /* UltiDB Operation successfully completed. */
    ULTIDB_RESULT_SUCCESS = 0,

    /* UltiDB operation error, e.g. storage full. */
    ULTIDB_RESULT_ERROR = 1,

    /* give more error codes here */

} ULTIDB_RESULT;

/** Chunking Strategy to choose from. When the data is large when integrating then it is automatically divided into
 * smaller pieces of data which we call chunks. This is done in order to increase the de-duplication ratio as smaller
 * data chunks lead to higher probability of de-duplication. If the type of data to be integrated is known beforehand
 * then a chunking strategy can be chosen accordingly. The chunking strategy to choose from are:
 * 1. Fixed Size
 * 2. Gear CDC
 * 3. Fast CDC
 * 4. Mod CDC
*/
typedef enum ULTIDB_CHUNKING_MODE {

    /**
     * Default chunking mode.
     *
     */
    ULTIDB_FIXED_SIZE_CHUNK = 0,

    /**  */
    ULTIDB_GEAR_CDC = 1,

    /**  */
    ULTIDB_FAST_CDC = 2,

    /** */
    ULTIDB_MOD_CDC = 3,

} ULTIDB_CHUNKING_MODE;

// ---------------------------------------------------------------------

/**
* Here goes configuration options that are usually exposed via client command line.
*/
ULTIDB_CONFIG* udb_create_config();

/**
 * Sets the connection parameter of the agency node in the given config file.
 *
 * @param cfg config file in which the connection parameters are set
 * @param hostname
 * @param port Port on which agency node is running
 * @param connection_pool The number of connections to create with the agency node.
 * @return bool true if the operation is successful, false if the operation failed
 */
ULTIDB_BOOL udb_config_set_agencynode(ULTIDB_CONFIG* cfg, const char *hostname, uint16_t port, size_t connection_pool = 3);

/**
 *
 * @param cfg config file in which the strategy is set
 * @param chunking_strategy The chunking strategy to apply
 * @return ULTIDB_BOOL true if the operation is successful, false if the operation failed
 */
ULTIDB_BOOL udb_config_set_chunking_strategy(ULTIDB_CONFIG* cfg, ULTIDB_CHUNKING_MODE chunking_strategy);

/**
 * Deallocates the intance created from ::udb_create_config.
 *
 * @param config config instance to be deallocated
 * @return ULTIDB_RESULT result of the operation
 */
ULTIDB_RESULT udb_destroy_config(ULTIDB_CONFIG *config);

/**
 * Creates an instance of ::ULTIDB_STATE_STRUCT (typedef ULTIDB) and initializes it.
 */
ULTIDB* udb_create_instance(ULTIDB_CONFIG *config);

/**
 * Destroys an instance of ::ULTIDB_STATE_STRUCT (typedef ULTIDB) and initializes it.
 *
 * @return ULTIDB_RESULT result of the operation
 */
ULTIDB_RESULT udb_destroy_instance(ULTIDB* ulti_db_instance);


/**
 * Return the error code for the last operation. The errors are saved per thread.
 */
uint8_t udb_get_errno(ULTIDB *db);
uint8_t udb_config_get_errno(ULTIDB_CONFIG *config);


// ---------------------------------------------------------------------

/**
 *
 * @param db ULTIDB object which can be used to perform operations such as integrate and retrieve.
 * @param data Data to integrate into the ULTIDB cluster
 * @return unsigned int of 8 bits The value 0 means the operation was successful, else a integer value is set according
 * to the error encountered. This error can be deduced from enum ::ULTIDB_RESULT.
 */
ULTIDB_RESULT udb_integrate(ULTIDB *db, const char* data);

/**
 * Data to retrieve from the ULTIDB cluster based on the hash provided.
 *
 * @param db ULTIDB object which can be used to perform operations such as integrate and retrieve.
 * @param udb_hash SHA512 hash of the data integrated
 * @return char* Returns nullptr if the operation is unsuccessful else a pointer to the underlying data is returned
 */
char* udb_retrieve(ULTIDB *db, const char* udb_hash);

/* !! 1. How to improve the interface such that integrate and retrieve have the same return value? That seems much more
 * consistent.
 * 2. Naming convention of the function so it's shorter.
*/


// ---------------------------------------------------------------------

#if defined(__cplusplus) || defined(c_plusplus)
} // extern "C" {
#endif

#endif /* ULTIDB_SDK_INCLUDE_ULTIDB */
