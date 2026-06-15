#pragma once
#include <proxy_define.h>
#include <functional>

namespace proxy
{
class proxy_server
{
public:
    proxy_server(tcp::endpoint endpoint);
    proxy_server(tcp::endpoint endpoint, std::function<void(tcp::socket)> handler);
    ~proxy_server();

    void set_receive_handler(std::function<void(tcp::socket)> handler);
    void run();
private:
    net::io_context io_context_;
    tcp::acceptor acceptor_;
    tcp::endpoint endpoint_;
    std::function<void(tcp::socket)> receive_handler_;
};
}