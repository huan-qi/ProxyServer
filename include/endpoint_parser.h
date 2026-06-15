#pragma once
#include <proxy_define.h>

namespace proxy
{

struct LocalIpv4 {
    std::string adapter_name;
    std::string ip;
    ULONG if_type;
    ULONG metric;
    int score;
};

class endpoint_parser
{
public:
    endpoint_parser();

    tcp::endpoint parse();

    std::vector<LocalIpv4> get_connectable_endpoint();

private:
    std::string wide_to_utf8(const wchar_t* wide_str);
    std::string to_lower(std::string s);
    bool contains_any(const std::string& s, const std::vector<std::string>& words);
    bool is_private_lan_ipv4(uint32_t host_order_ip);
    bool is_bad_ipv4(uint32_t host_order_ip);
};

}

