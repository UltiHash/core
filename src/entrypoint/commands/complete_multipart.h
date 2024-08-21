#ifndef ENTRYPOINT_HTTP_COMPLETE_MULTIPART_H
#define ENTRYPOINT_HTTP_COMPLETE_MULTIPART_H

#include "command.h"
#include "common/global_data/global_data_view.h"
#include "entrypoint/directory.h"
#include "entrypoint/limits.h"
#include "entrypoint/multipart_state.h"

namespace uh::cluster {

class complete_multipart : public command {
public:
    complete_multipart(directory&, global_data_view&, multipart_state&,
                       limits&);

    static bool can_handle(const http_request& req);

    coro<http_response> handle(http_request& req) override;

private:
    static void validate_internal(const upload_info& info,
                                  std::span<char> body);
    coro<bool> get_old_object(http_request& req, object& old_obj);

    directory& m_directory;
    global_data_view& m_gdv;
    multipart_state& m_uploads;
    limits& m_limits;
    static constexpr std::size_t MAXIMUM_CHUNK_SIZE = 5ul * 1024ul * 1024ul;
    static constexpr std::size_t MAXIMUM_PART_NUMBER = 10000;
};

} // namespace uh::cluster

#endif
