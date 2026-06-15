#include "proxy_server.h"
#include <spdlog/spdlog.h>

namespace proxy
{
proxy_server::proxy_server(tcp::endpoint endpoint)
    : acceptor_(io_context_),
      endpoint_(endpoint) { }

proxy_server::proxy_server(tcp::endpoint endpoint, std::function<void(tcp::socket)> handler)
    : proxy_server(endpoint)
{
    receive_handler_ = handler;
};

proxy_server::~proxy_server() = default;

void proxy_server::set_receive_handler(std::function<void(tcp::socket)> handler)
{
    receive_handler_ = handler;
}

void proxy_server::run()
{
    acceptor_.open(endpoint_.protocol());
    acceptor_.set_option(net::socket_base::reuse_address(true));
    acceptor_.bind(endpoint_);
    acceptor_.listen(net::socket_base::max_listen_connections);

    spdlog::info("Proxy server is running...");

    for (;;) 
    {
        tcp::socket socket(io_context_);
        acceptor_.accept(socket);

        spdlog::info("Receive connect...");
        std::thread([this, sock = std::move(socket)]() mutable {
            if (receive_handler_)
            {
                receive_handler_(std::move(sock));
            }
        }).detach();
    }
}
}
