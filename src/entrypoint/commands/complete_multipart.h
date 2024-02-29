#ifndef ENTRYPOINT_HTTP_COMPLETE_MULTIPART_H
#define ENTRYPOINT_HTTP_COMPLETE_MULTIPART_H

#include "entrypoint/http/http_request.h"
#include "entrypoint/http/http_response.h"
#include "entrypoint/utils.h"

namespace uh::cluster {

class complete_multipart {
public:
    explicit complete_multipart(reference_collection&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) const;

private:
    reference_collection& m_collection;
    md5 m_md5;
    static constexpr std::size_t MAXIMUM_CHUNK_SIZE = 5ul * 1024ul * 1024ul;
    static constexpr std::size_t MAXIMUM_PART_NUMBER = 10000;

    void validate(const http_request& req) const;
};

} // namespace uh::cluster

#endif
