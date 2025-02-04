#pragma once

namespace uh::cluster::ep::cors {

struct cors_info {
    std::string allowed_origin;

    bool allowed_delete = false;
    bool allowed_get = false;
    bool allowed_head = false;
    bool allowed_post = false;
    bool allowed_put = false;
};

class parser {
public:
    static std::map<std::string, cors_info> parse(std::string code);
};

} // namespace uh::cluster::ep::cors
