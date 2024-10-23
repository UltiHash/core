#ifndef CORE_ENTRYPOINT_HTTP_RANGE_H
#define CORE_ENTRYPOINT_HTTP_RANGE_H

#include <common/types/address.h>

#include <list>
#include <string>

namespace uh::cluster::ep::http {

struct range_spec {
    enum unit_t { bytes };

    struct range {
        // index of first byte
        std::size_t start = 0ull;
        // index directly after the last byte
        std::size_t end = 0ull;

        std::size_t length() const;
    };

    std::list<range> ranges;
    unit_t unit;
};

/**
 * Parse an HTTP range.
 *
 * @param header value of the HTTP `Range` header
 * @param max maximum value in the range, used to compute negative ranges
 */
range_spec parse_range_header(std::string_view header, std::size_t max);

/**
 * Parse a single range specifier
 */
range_spec::range parse_range_spec(std::string_view header, std::size_t max);

/**
 * Apply the range specification to the given address.
 */
address apply_range(address addr, const range_spec& spec);

} // namespace uh::cluster::ep::http

#endif
