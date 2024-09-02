#ifndef CORE_ENTRYPOINT_HTTP_DEFAULT_BODY_FACTORY_H
#define CORE_ENTRYPOINT_HTTP_DEFAULT_BODY_FACTORY_H

#include "request_factory.h"

namespace uh::cluster::ep::http {

class default_body_factory : public body_factory {
public:
    std::unique_ptr<body> create(partial_parse_result& req) override;
};

} // namespace uh::cluster::ep::http

#endif
