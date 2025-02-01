#pragma once

#include <functional>
#include <memory>
#include <string>

class http_server {
public:
    using GetHandler = std::function<std::string()>;
    using PostHandler = std::function<std::string(const std::string& body)>;

    http_server(int port, const std::string& user, const std::string& passwd);
    ~http_server();

    void set_get_handler(const std::string& path, GetHandler handler);
    void set_post_handler(const std::string& path, PostHandler handler);

private:
    class impl;
    std::unique_ptr<impl> pImpl;
};
