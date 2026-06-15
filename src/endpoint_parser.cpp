#include "endpoint_parser.h"

#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <algorithm>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace proxy
{
endpoint_parser::endpoint_parser() { }

tcp::endpoint endpoint_parser::parse()
{
    auto address = net::ip::make_address("0.0.0.0");
    ip::port_type port = 8080;
    return tcp::endpoint{address, port};
}

std::vector<LocalIpv4> endpoint_parser::get_connectable_endpoint()
{
    std::vector<LocalIpv4> result;

    ULONG flags =
        GAA_FLAG_SKIP_ANYCAST |
        GAA_FLAG_SKIP_MULTICAST |
        GAA_FLAG_SKIP_DNS_SERVER;

    ULONG size = 16 * 1024;
    std::vector<unsigned char> buffer(size);

    IP_ADAPTER_ADDRESSES* adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

    ULONG ret = GetAdaptersAddresses(AF_INET, flags, nullptr, adapters, &size);

    if (ret == ERROR_BUFFER_OVERFLOW) 
    {
        buffer.resize(size);
        adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        ret = GetAdaptersAddresses(AF_INET, flags, nullptr, adapters, &size);
    }

    if (ret != NO_ERROR) 
    {
        return result;
    }

    std::vector<std::string> virtual_keywords = {
        "virtual",
        "vmware",
        "virtualbox",
        "hyper-v",
        "docker",
        "wsl",
        "tap",
        "tun",
        "vpn",
        "tailscale",
        "zerotier",
        "loopback"
    };

    for (auto* adapter = adapters; adapter != nullptr; adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp) {
            continue;
        }

        if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK ||
            adapter->IfType == IF_TYPE_TUNNEL ||
            adapter->IfType == IF_TYPE_PPP) {
            continue;
        }

        std::string name = wide_to_utf8(adapter->FriendlyName);
        std::string desc = wide_to_utf8(adapter->Description);
        std::string combined = to_lower(name + " " + desc);

        if (contains_any(combined, virtual_keywords)) {
            continue;
        }

        for (auto* addr = adapter->FirstUnicastAddress; addr != nullptr; addr = addr->Next) {
            if (!addr->Address.lpSockaddr) {
                continue;
            }

            if (addr->Address.lpSockaddr->sa_family != AF_INET) {
                continue;
            }

            auto* sockaddr = reinterpret_cast<sockaddr_in*>(addr->Address.lpSockaddr);
            uint32_t host_ip = ntohl(sockaddr->sin_addr.s_addr);

            if (is_bad_ipv4(host_ip)) {
                continue;
            }

            if (!is_private_lan_ipv4(host_ip)) {
                continue;
            }

            char ip_buffer[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, &sockaddr->sin_addr, ip_buffer, sizeof(ip_buffer));

            int score = 0;

            if (adapter->IfType == IF_TYPE_IEEE80211) {
                score += 1000; // Wi-Fi 优先
            } else if (adapter->IfType == IF_TYPE_ETHERNET_CSMACD) {
                score += 900;  // 有线网卡
            }

            score -= static_cast<int>(std::min<ULONG>(adapter->Ipv4Metric, 500));

            result.push_back(LocalIpv4{
                name,
                ip_buffer,
                adapter->IfType,
                adapter->Ipv4Metric,
                score
            });
        }
    }

    std::sort(result.begin(), result.end(), [](const LocalIpv4& a, const LocalIpv4& b) {
        return a.score > b.score;
    });

    return result;
}

std::string endpoint_parser::wide_to_utf8(const wchar_t* wide_str)
{
    if (!wide_str) return "";

    int size = WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, nullptr, 0, nullptr, nullptr);

    if (size <= 0) return "";

    std::string result(size - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide_str, -1, result.data(), size, nullptr, nullptr);
    return result;
}

std::string endpoint_parser::to_lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [] (unsigned char c) 
    {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

bool endpoint_parser::contains_any(const std::string& s, const std::vector<std::string>& words)
{
    for (const auto& word : words)
    {
        if (s.find(word) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

bool endpoint_parser::is_private_lan_ipv4(uint32_t host_order_ip)
{
    // 10.0.0.0/8
    if ((host_order_ip & 0xFF000000) == 0x0A000000) return true;

    // 172.16.0.0/12
    if ((host_order_ip & 0xFFF00000) == 0xAC100000) return true;

    // 192.168.0.0/16
    if ((host_order_ip & 0xFFFF0000) == 0xC0A80000) return true;

    return false;
}

bool endpoint_parser::is_bad_ipv4(uint32_t host_order_ip)
{
    // 0.0.0.0
    if (host_order_ip == 0) return true;

    // 127.0.0.0/8
    if ((host_order_ip & 0xFF000000) == 0x7F000000) return true;

    // 169.254.0.0/16 link-local
    if ((host_order_ip & 0xFFFF0000) == 0xA9FE0000) return true;

    return false;
}

}