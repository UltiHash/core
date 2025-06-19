#include "put_bucket_versioning.h"

#include <common/utils/xml_parser.h>


using namespace uh::cluster::ep::http;

namespace uh::cluster {

put_bucket_versioning::put_bucket_versioning(directory& dir)
    : m_dir(dir) {}

bool put_bucket_versioning::can_handle(const ep::http::request& req) {
    return req.method() == verb::put && !req.bucket().empty() &&
           req.bucket() != RESERVED_BUCKET_NAME && req.object_key().empty() &&
           req.query("versioning");
}

coro<response> put_bucket_versioning::handle(request& req) {

    unique_buffer<char> buffer(req.content_length());
    auto size = co_await req.read_body(buffer.span());
    buffer.resize(size);

    xml_parser xml_parser;
    bool parsed = xml_parser.parse({&*buffer.begin(), buffer.size()});
    auto part_nodes = xml_parser.get_nodes("VersioningConfiguration.Status");

    if (!parsed || part_nodes.size() != 1) {
        throw command_exception(status::bad_request, "MalformedXML",
            "The XML that you provided was not well formed "
            "or did not validate against our published schema.");
    }

    auto status = to_versioning(part_nodes[0].get().get_value<std::string>());
    if (status != bucket_versioning::enabled && status != bucket_versioning::suspended) {
        throw command_exception(status::bad_request, "MalformedXML",
            "The XML that you provided was not well formed "
            "or did not validate against our published schema.");
    }

    co_await m_dir.set_bucket_versioning(req.bucket(), status);
    co_return response();
}

std::string put_bucket_versioning::action_id() const { return "s3:PutBucketVersioning"; }

} // namespace uh::cluster
