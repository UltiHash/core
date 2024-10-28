#ifndef CORE_ENTRYPOINT_CONSTANT_H
#define CORE_ENTRYPOINT_CONSTANT_H

namespace uh::cluster::ep {

/**
 * Content-Type to assign to objects when none was specified in the request.
 */
constexpr const char* DEFAULT_OBJECT_CONTENT_TYPE = "binary/octet-stream";

} // namespace uh::cluster::ep

#endif
