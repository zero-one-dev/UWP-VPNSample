#include <unordered_map>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <string>

class ClientIpMap
{
    public:
        ClientIpMap(uint32_t client_vpn_ip, sockaddr_in *sockaddr, int32_t addr_len)
        {
            this->client_vpn_ip = client_vpn_ip;
            ApplyNew(sockaddr, addr_len);
        }

        void UpdateIpMap(sockaddr_in *sockaddr, int32_t addr_len)
        {
            if ((this->sockaddr_len == addr_len) && (memcmp(sockaddr, &(this->client_udp_container_sockaddr), addr_len) == 0))
            {
                return;
            }
            ApplyNew(sockaddr, addr_len);
        }

        struct sockaddr_in& GetSockAddrIn()
        {
            return client_udp_container_sockaddr;
        }

        int32_t GetSockAddrInLen()
        {
            return sockaddr_len;
        }
        

    private: 
        void ApplyNew(sockaddr_in *sockaddr, int32_t addr_len)
        {
            printf("get new client\n");
            memcpy(&(this->client_udp_container_sockaddr), sockaddr, addr_len);            
            sockaddr_len = addr_len;
        }

        uint32_t client_vpn_ip;
        
        struct sockaddr_in client_udp_container_sockaddr;
        int32_t sockaddr_len;

        int64_t last_active_time;   // not used yet
              
};


// only support ipv4
class Clients
{
public:
    ClientIpMap* get_client_by_ipv4(uint32_t ipv4)
    {
        auto it = m_vpn_ip_2_udp_ip.find(ipv4);
        if (it == m_vpn_ip_2_udp_ip.end())
        {
            return nullptr;
        }
        return it->second;
    }

    ClientIpMap* set_client_by_ipv4(uint32_t ipv4, sockaddr_storage *psock_addr, int sockaddr_len)
    {
        auto it = m_vpn_ip_2_udp_ip.find(ipv4);
        if (it == m_vpn_ip_2_udp_ip.end())
        {
            ClientIpMap* pClientIpMap = new ClientIpMap(ipv4, (sockaddr_in*)psock_addr, sockaddr_len);
            m_vpn_ip_2_udp_ip[ipv4] = pClientIpMap;
            return pClientIpMap;
        }
        else
        {
            it->second->UpdateIpMap((sockaddr_in*)psock_addr, sockaddr_len);
            return it->second;
        }
    }

private:
    std::unordered_map<uint32_t, ClientIpMap*> m_vpn_ip_2_udp_ip;
};
