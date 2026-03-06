/*
 * Copyright 2026 UltiHash Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <common/types/common_types.h>

#include <iosfwd>
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

/**
 * Input a timestamp in ISO 8601 time format, as described above.
 */
utc_time read_iso8601_date(std::string_view str);

/**
 * Input a timestamp in ISO 8601 time format (YYYYMMDDTHHMMSSZ)
 */
utc_time read_iso8601_date_merged(std::string_view s);

utc_time make_utc_time(int year, int month, int day, int hour, int min,
                       int sec);

namespace detail {

utc_time read_local_date(std::string_view str);
std::chrono::hours read_timezone(std::string_view str);

} // namespace detail

} // namespace uh::cluster

namespace std {

ostream& operator<<(ostream& out, const uh::cluster::utc_time& t);

}
