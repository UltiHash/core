#ifndef CORE_ENTRYPOINT_FORMATS_H
#define CORE_ENTRYPOINT_FORMATS_H

#include "common/types/common_types.h"

#include <string>

namespace uh::cluster {

/**
 * Output a timestamp in IMF fixdate format as defined in
 * https://datatracker.ietf.org/doc/html/rfc9110#name-date-time-formats
 *
 * Sample:  Sun, 06 Nov 1994 08:49:37 GMT
 */
std::string imf_fixdate(const utc_time& ts);

/**
 * Output a timestamp in ISO 8601 time format as defined in
 * http://tools.ietf.org/html/rfc3339
 *
 * Sample: 2009-10-12T17:50:30.000Z
 */
std::string iso8601_date(const utc_time& ts);

} // namespace uh::cluster

#endif
