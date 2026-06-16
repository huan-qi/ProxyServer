#include <iostream>
#include <endpoint_parser.h> 
#include <proxy_server.h>
#include <http_proxy.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>
#include <boost/asio.hpp>

using namespace proxy;

const size_t max_file_size = 1024 * 1024 * 20;
const size_t max_files = 5;

void init_log();

int main(int argc, char** argv)
{
    init_log();

    spdlog::info("Starting proxy server...");

    endpoint_parser ep_parser;
    auto endpoint = ep_parser.parse();
    auto addresses =  ep_parser.get_lan_ipv4_addresses();

    for (auto address : addresses)
    {
        spdlog::info("Proxy server listening on {}: {}", address.to_string(), endpoint.port());
    }

    proxy_server server(endpoint);
    server.set_receive_handler([] (tcp::socket socket) {
        http_proxy hp(std::move(socket));
        hp.handle();       
    });
    server.run();
    
    spdlog::shutdown();

    return 0;
}

void init_log()
{
    spdlog::init_thread_pool(8192, 1); // 队列大小8192，线程数1

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::info);

    auto logger = spdlog::create_async<spdlog::sinks::rotating_file_sink_mt>(
        "server_logger",
        "logs/server.log",
        max_file_size,
        max_files);

    logger->sinks().push_back(console_sink);

    logger->set_level(spdlog::level::trace);

    spdlog::set_default_logger(logger);
}