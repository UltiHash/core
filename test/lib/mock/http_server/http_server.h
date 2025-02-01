#pragma once

#include <functional>
#include <httplib.h>
#include <memory>
#include <string>

class http_server {
public:
    using GetHandler = std::function<void(httplib::Response& resp)>;
    using PostHandler = std::function<void(const httplib::Request& req,
                                           httplib::Response& resp)>;

    http_server(const std::string& user, const std::string& passwd);
    ~http_server();

    void set_get_handler(const std::string& path, GetHandler handler);
    void set_post_handler(const std::string& path, PostHandler handler);
    int get_port() const;

private:
    class impl;
    std::unique_ptr<impl> pImpl;
};
