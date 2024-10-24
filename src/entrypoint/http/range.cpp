#include "range.h"

#include <common/telemetry/log.h>
#include <common/utils/strings.h>
#include <entrypoint/http/command_exception.h>

namespace uh::cluster::ep::http {

std::size_t range_spec::range::length() const {
    return end < start ? 0 : end - start;
}

range_spec parse_range_header(std::string_view header, std::size_t max) {
    auto equals = header.find('=');
    if (equals == std::string::npos) {
        throw command_exception(status::bad_request, "BadRequest",
                                "cannot parse range specifier");
    }

    if (lowercase(std::string(header.substr(0, equals))) != "bytes") {
        throw command_exception(status::bad_request, "BadRequest",
                                "unsupported range unit");
    }

    auto specs = split(header.substr(equals + 1), ',');

    std::list<range_spec::range> ranges;
    for (const auto& spec : specs) {
        ranges.emplace_back(parse_range_spec(spec, max));
    }

    return range_spec{.ranges = std::move(ranges), .unit = range_spec::bytes};
}

range_spec::range parse_range_spec(std::string_view spec, std::size_t size) {
    auto dash = spec.find('-');
    if (dash == std::string::npos) {
        throw command_exception(status::bad_request, "BadRequest",
                                "cannot parse range specifier");
    }

    try {
        if (dash == 0) {
            auto bytes = std::stoull(std::string(spec.substr(1)));
            return range_spec::range{size - bytes, size};
        }

        if (dash == spec.size() - 1) {
            auto bytes =
                std::stoull(std::string(spec.substr(0, spec.size() - 1)));
            return range_spec::range{bytes, size};
        }

        auto from = std::stoull(std::string(spec.substr(0, dash)));
        auto to = std::stoull(std::string(spec.substr(dash + 1))) + 1;

        return range_spec::range{from, to};
    } catch (const std::exception& e) {
        LOG_DEBUG() << "error parsing `" << spec << "`: " << e.what();
        throw command_exception(status::bad_request, "BadRequest",
                                "cannot parse range specifier");
    }

    return {};
}

address apply_range(address addr, const range_spec& spec) {
    if (addr.size() == 0) {
        return addr;
    }

    address rv;
    for (const auto& range : spec.ranges) {
        rv.append(addr.range(range.start, range.end));
    }

    return rv;
}

} // namespace uh::cluster::ep::http
