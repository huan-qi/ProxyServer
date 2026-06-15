#include "http_proxy.h"
#include <spdlog/spdlog.h>

namespace proxy
{
constexpr std::uint16_t kDefaultPort = 8080;
constexpr std::size_t kBufferSize = 16 * 1024;

http_proxy::http_proxy(tcp::socket socket) : 
    client_(std::move(socket)) { }

void http_proxy::handle()
{
    try
    {
        beast::flat_buffer buffer;
        http::request<http::string_body> request;

        request.body().reserve(8192);
        http::read(client_, buffer, request);

        if (request.method() == http::verb::connect)
        {
            handle_connect(request, buffer);
        }
        else
        {
            handle_http(request);
        }
    }
    catch (const std::exception &ex)
    {
        spdlog::error("Error handling request: {}", ex.what());
        send_error(http::status::bad_gateway, ex.what());
    }
}

void http_proxy::handle_http(http::request<http::string_body> request)
{
    spdlog::info("Handling HTTP request: {} {}", request.method_string(), request.target());
    
    auto destination = parse_http_destination(request);
    tcp::socket remote(client_.get_executor());
    tcp::resolver resolver(client_.get_executor());

    request.target(destination.target);
    request.set(http::field::host, destination.host_header);
    request.set(http::field::connection, "close");
    request.erase(http::field::proxy_connection);
    request.erase(http::field::proxy_authorization);

    net::connect(remote, resolver.resolve(destination.host, destination.port));
    http::write(remote, request);

    relay(remote, client_);
}

void http_proxy::handle_connect(const http::request<http::string_body> &request,
                                beast::flat_buffer &buffered_client_data)
{
    spdlog::info("Handling CONNECT request: {}", request.target());

    auto destination = parse_connect_destination(request);
    tcp::socket remote(client_.get_executor());
    tcp::resolver resolver(client_.get_executor());

    net::connect(remote, resolver.resolve(destination.host, destination.port));

    static constexpr char response[] =
        "HTTP/1.1 200 Connection Established\r\n"
        "Proxy-Agent: boost-asio-beast-proxy\r\n"
        "\r\n";

    net::write(client_, net::buffer(response, sizeof(response) - 1));

    if (buffered_client_data.size() > 0)
    {
        net::write(remote, buffered_client_data.data());
        buffered_client_data.consume(buffered_client_data.size());
    }

    std::thread upstream([&]
                         { relay(client_, remote); });
    relay(remote, client_);
    upstream.join();
}

void http_proxy::send_error(http::status status, const std::string &message)
{
    boost::system::error_code ignored;

    http::response<http::string_body> response{status, 1};
    response.set(http::field::server, "boost-asio-beast-proxy");
    response.set(http::field::content_type, "text/plain; charset=utf-8");
    response.keep_alive(false);
    response.body() = message;
    response.prepare_payload();

    http::write(client_, response, ignored);
}

void http_proxy::relay(tcp::socket& input, tcp::socket& output)
{
    std::array<char, kBufferSize> buffer{};
    boost::system::error_code ec;

    for (;;) 
    {
        auto bytes_read = input.read_some(net::buffer(buffer), ec);
        if (ec) 
        {
            break;
        }

        net::write(output, net::buffer(buffer.data(), bytes_read), ec);
        if (ec) 
        {
            break;
        }
    }

    close_socket(input);
    close_socket(output);
}

void http_proxy::close_socket(tcp::socket& socket)
{
    boost::system::error_code ignored;
    socket.shutdown(tcp::socket::shutdown_both, ignored);
    socket.close(ignored);
}

Destination http_proxy::parse_connect_destination(const http::request<http::string_body> &request)
{
    auto authority = to_string(request.target());
    auto [host, port] = split_host_port(authority, "443");

    if (host.empty() || port.empty()) {
        throw std::runtime_error("CONNECT target must be host:port");
    }

    return {host, port, make_host_header(host, port, "443"), authority};
}

Destination http_proxy::parse_http_destination(http::request<http::string_body> const& request)
{
    auto target = to_string(request.target());
    std::string default_port = "80";
    std::string authority;
    std::string origin_target;

    if (starts_with_ci(target, "http://") || starts_with_ci(target, "https://")) {
        auto scheme_end = target.find("://");
        auto rest_begin = scheme_end + 3;
        auto path_begin = target.find('/', rest_begin);
        authority = path_begin == std::string::npos
                        ? target.substr(rest_begin)
                        : target.substr(rest_begin, path_begin - rest_begin);
        origin_target = path_begin == std::string::npos ? "/" : target.substr(path_begin);
        default_port = starts_with_ci(target, "https://") ? "443" : "80";
    } else {
        auto host_header = request[http::field::host];
        if (host_header.empty()) {
            throw std::runtime_error("HTTP request is missing Host header");
        }
        authority = to_string(host_header);
        origin_target = target.empty() ? "/" : target;
    }

    auto [host, port] = split_host_port(authority, default_port);
    if (host.empty() || port.empty()) {
        throw std::runtime_error("invalid HTTP destination");
    }

    return {host, port, make_host_header(host, port, default_port), origin_target};
}

std::string http_proxy::to_string(beast::string_view value)
{
    return {value.data(), value.size()};
}

bool http_proxy::starts_with_ci(std::string const& value, std::string const& prefix)
{
    if (value.size() < prefix.size()) {
        return false;
    }

    for (std::size_t i = 0; i < prefix.size(); ++i) {
        auto lhs = static_cast<unsigned char>(value[i]);
        auto rhs = static_cast<unsigned char>(prefix[i]);
        if (std::tolower(lhs) != std::tolower(rhs)) {
            return false;
        }
    }

    return true;
}

std::pair<std::string, std::string> http_proxy::split_host_port(std::string authority,
                                                    std::string default_port)
{
    auto at = authority.rfind('@');
    if (at != std::string::npos) {
        authority.erase(0, at + 1);
    }

    if (!authority.empty() && authority.front() == '[') {
        auto end = authority.find(']');
        if (end == std::string::npos) {
            throw std::runtime_error("invalid IPv6 authority: " + authority);
        }

        auto host = authority.substr(1, end - 1);
        auto port = default_port;
        if (end + 1 < authority.size()) {
            if (authority[end + 1] != ':') {
                throw std::runtime_error("invalid IPv6 authority: " + authority);
            }
            port = authority.substr(end + 2);
        }
        return {host, port};
    }

    auto first_colon = authority.find(':');
    auto last_colon = authority.rfind(':');
    if (first_colon != std::string::npos && first_colon == last_colon) {
        return {authority.substr(0, first_colon), authority.substr(first_colon + 1)};
    }

    return {authority, default_port};
}


std::string http_proxy::make_host_header(std::string const& host, std::string const& port,
                             std::string const& default_port) 
{
    auto needs_brackets = host.find(':') != std::string::npos &&
                          !(host.size() >= 2 && host.front() == '[' && host.back() == ']');
    std::string value = needs_brackets ? "[" + host + "]" : host;
    if (!port.empty() && port != default_port) {
        value += ":" + port;
    }
    return value;
}

}

