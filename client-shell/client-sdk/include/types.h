/**
 * @file
 * Common types used in the ULTIDB API.
 */

#ifndef ULTIDB_SDK_INCLUDE_TYPES_H
#define ULTIDB_SDK_INCLUDE_TYPES_H

#include <cstddef>  /* for size_t */
#include <cstdint>

#define ULTIDB_BOOL int
#define ULTIDB_TRUE 1
#define ULTIDB_FALSE 0
#define TO_ULTIDB_BOOL(X) (!!(X) ? ULTIDB_TRUE : ULTIDB_FALSE)

#endif /* ULTIDB_SDK_INCLUDE_TYPES_H  */
