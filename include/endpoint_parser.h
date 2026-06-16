#pragma once
#include <proxy_define.h>
#include <string>
#include <vector>

namespace proxy
{
using boost::asio::ip::address_v4;

class endpoint_parser
{
public:
    endpoint_parser();

    tcp::endpoint parse();

    std::vector<address_v4> get_lan_ipv4_addresses();

private:
    bool is_bad_ipv4(const address_v4& addr);
    bool starts_with(const std::string& str, const std::string& prefix);
    bool is_virtual_interface_name(const std::string& name);
};

}

