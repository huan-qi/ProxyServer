#include "endpoint_parser.h"

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <iphlpapi.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <arpa/inet.h>
#  include <ifaddrs.h>
#  include <net/if.h>
#  include <netinet/in.h>
#endif

namespace proxy
{
endpoint_parser::endpoint_parser() { }

tcp::endpoint endpoint_parser::parse()
{
    auto address = net::ip::make_address("0.0.0.0");
    ip::port_type port = 8080;
    return tcp::endpoint{address, port};
}

std::vector<address_v4> endpoint_parser::get_lan_ipv4_addresses()
{
    std::vector<address_v4> result;

#ifdef _WIN32

    ULONG flags = GAA_FLAG_SKIP_ANYCAST |
                  GAA_FLAG_SKIP_MULTICAST |
                  GAA_FLAG_SKIP_DNS_SERVER;

    ULONG buffer_size = 15 * 1024;

    std::vector<unsigned char> buffer(buffer_size);

    IP_ADAPTER_ADDRESSES* adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

    ULONG ret = GetAdaptersAddresses(AF_INET, flags, nullptr, adapters, &buffer_size);

    if (ret == ERROR_BUFFER_OVERFLOW) 
    {
        buffer.resize(buffer_size);
        adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        ret = GetAdaptersAddresses(AF_INET, flags, nullptr, adapters, &buffer_size);
    }

    if (ret != NO_ERROR) 
    {
        return result;
    }

    for (auto* adapter = adapters; adapter != nullptr; adapter = adapter->Next) 
    {
        if (adapter->OperStatus != IfOperStatusUp) 
        {
            continue;
        }

        if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK) 
        {
            continue;
        }

        if (adapter->IfType == IF_TYPE_TUNNEL) 
        {
            continue;
        }

        std::wstring friendly_name = adapter->FriendlyName ? adapter->FriendlyName : L"";

        std::string adapter_name = adapter->AdapterName ? adapter->AdapterName : "";

        if (is_virtual_interface_name(adapter_name)) 
        {
            continue;
        }

        for (auto* unicast = adapter->FirstUnicastAddress; unicast != nullptr; unicast = unicast->Next) 
        {
            auto* sa = unicast->Address.lpSockaddr;

            if (!sa || sa->sa_family != AF_INET) 
            {
                continue;
            }

            auto* addr_in = reinterpret_cast<sockaddr_in*>(sa);

            address_v4 addr(ntohl(addr_in->sin_addr.s_addr));

            if (is_bad_ipv4(addr)) 
            {
                continue;
            }

            result.push_back(addr);
        }
    }

#else


    ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1) {
        return result;
    }

    for (ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }

        if (ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        std::string ifname = ifa->ifa_name;

        if (is_virtual_interface_name(ifname)) {
            continue;
        }

        unsigned int flags = ifa->ifa_flags;

        if (!(flags & IFF_UP)) {
            continue;
        }

        if (flags & IFF_LOOPBACK) {
            continue;
        }

#ifdef IFF_RUNNING
        if (!(flags & IFF_RUNNING)) {
            continue;
        }
#endif

        auto* sa = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);

        address_v4 addr(ntohl(sa->sin_addr.s_addr));

        if (is_bad_ipv4(addr)) {
            continue;
        }

        if (!is_private_ipv4(addr)) {
            continue;
        }

        result.push_back(addr);
    }

    freeifaddrs(ifaddr);
    
#endif

    return result;
}

bool endpoint_parser::is_bad_ipv4(const address_v4& addr)
{
    auto bytes = addr.to_bytes();
    address_v4::any();
    return addr == address_v4::any() ||
            addr.is_loopback() ||
            addr.is_multicast() ||
            (bytes[0] == 0) || 
            (bytes[0] == 127) || 
            (bytes[0] == 169 && bytes[1] == 254);

}

bool endpoint_parser::starts_with(const std::string& str, const std::string& prefix)
{
    return str.rfind(prefix, 0) == 0;
}

bool endpoint_parser::is_virtual_interface_name(const std::string& name)
{
   static const std::vector<std::string> prefixes = {
        "lo", "docker", "veth", "br-", "virbr", "vmnet",
        "tun", "tap", "wg", "tailscale", "zt", "cni",
        "flannel", "kube"
    };

    for (const auto& prefix : prefixes) 
    {
        if (starts_with(name, prefix))
        {
            return true;
        }
    }
    return false;
}



}