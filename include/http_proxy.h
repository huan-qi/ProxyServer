#include <proxy_define.h>

namespace proxy
{

struct AbsoluteUri {
    std::string authority;
    std::string origin_target;
    std::string default_port;
};

struct Destination
{
    std::string host;
    std::string port;
    std::string host_header;
    std::string target;
};

class http_proxy
{
public:
    http_proxy(tcp::socket socket);

    void handle();

private:
    void handle_http(http::request<http::string_body> request);
    void handle_connect(const http::request<http::string_body> &request,
                        beast::flat_buffer &buffered_client_data);
    void send_error(http::status status, const std::string &message);
    void relay(tcp::socket& input, tcp::socket& output);
    void close_socket(tcp::socket& socket);

    Destination parse_connect_destination(const http::request<http::string_body> &request);
    Destination parse_http_destination(http::request<http::string_body> const& request);

    std::string to_string(beast::string_view value);
    
    bool starts_with_ci(std::string const& value, std::string const& prefix);

    std::pair<std::string, std::string> split_host_port(std::string authority,
                                                    std::string default_port);

    std::string make_host_header(std::string const& host, std::string const& port,
                                 std::string const& default_port);

private:
    tcp::socket client_;
};
}