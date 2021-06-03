#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <thread>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <map>
#include <vector>

#include "Clients.h"

/*
 * The following lines serve as configurations
 * Uncomment first 2 lines to run as vpn client
 */

// #define AS_CLIENT YES
// #define SERVER_HOST ""

#define DEBUG_OUTPUT 0

#define PORT 10000
#define MTU 1400
#define READ_BUFFER_LEN 2 * MTU
#define BIND_HOST "0.0.0.0"

std::string exe_dir;

static int max(int a, int b)
{
    return a > b ? a : b;
}

/*
 * Create VPN interface /dev/tun0 and return a fd
 */
int tun_alloc()
{
    struct ifreq ifr;
    int fd, e;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
    {
        perror("Cannot open /dev/net/tun");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, "tun0", IFNAMSIZ);

    if ((e = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0)
    {
        perror("ioctl[TUNSETIFF]");
        close(fd);
        return e;
    }

    return fd;
}

/*
 * Execute commands
 */
static void run(const char *cmd, bool exit_on_error = true)
{
    printf("Execute `%s`\n", cmd);
    if (system(cmd))
    {
        perror(cmd);
        if (exit_on_error)
        {
            exit(1);
        }
    }
}

/*
 * Configure IP address and MTU of VPN interface /dev/tun0
 */
void ifconfig()
{
    char cmd[1024];

#ifdef AS_CLIENT
    snprintf(cmd, sizeof(cmd), "ifconfig tun0 10.0.0.2/8 mtu %d up", MTU);
#else
    snprintf(cmd, sizeof(cmd), "ifconfig tun0 10.0.0.1/8 mtu %d up", MTU);
#endif
    run(cmd);
}

/*
 * Setup route table via `iptables` & `ip route`
 */
void setup_route_table()
{
    run("sysctl -w net.ipv4.ip_forward=1");

#ifdef AS_CLIENT
    run("iptables -t nat -A POSTROUTING -o tun0 -j MASQUERADE");
    run("iptables -I FORWARD 1 -i tun0 -m state --state RELATED,ESTABLISHED -j ACCEPT");
    run("iptables -I FORWARD 1 -o tun0 -j ACCEPT");
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "ip route add %s via $(ip route show 0/0 | sed -e 's/.* via \([^ ]*\).*/\1/')", SERVER_HOST);
    run(cmd);
    run("ip route add 0/1 dev tun0");
    run("ip route add 128/1 dev tun0");
#else
    run("iptables -t nat -A POSTROUTING -s 10.0.0.0/8 ! -d 10.0.0.0/8 -m comment --comment 'vpndemo' -j MASQUERADE");
    run("iptables -A FORWARD -s 10.0.0.0/8 -m state --state RELATED,ESTABLISHED -j ACCEPT");
    run("iptables -A FORWARD -d 10.0.0.0/8 -j ACCEPT");
#endif
}

/*
 * Cleanup route table
 */
void cleanup_route_table()
{
#ifdef AS_CLIENT
    run("iptables -t nat -D POSTROUTING -o tun0 -j MASQUERADE");
    run("iptables -D FORWARD -i tun0 -m state --state RELATED,ESTABLISHED -j ACCEPT");
    run("iptables -D FORWARD -o tun0 -j ACCEPT");
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "ip route del %s", SERVER_HOST);
    run(cmd);
    run("ip route del 0/1");
    run("ip route del 128/1");
#else
    run("iptables -t nat -D POSTROUTING -s 10.0.0.0/8 ! -d 10.0.0.0/8 -m comment --comment 'vpndemo' -j MASQUERADE", false);
    run("iptables -D FORWARD -s 10.0.0.0/8 -m state --state RELATED,ESTABLISHED -j ACCEPT", false);
    run("iptables -D FORWARD -d 10.0.0.0/8 -j ACCEPT", false);
#endif
}

/*
 * Bind UDP port
 */
int udp_bind(struct sockaddr *addr, socklen_t *addrlen, uint16_t port)
{
    struct addrinfo hints;
    struct addrinfo *result;
    int sock, flags;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

#ifdef AS_CLIENT
    const char *host = SERVER_HOST;
#else
    const char *host = BIND_HOST;
#endif
    if (0 != getaddrinfo(host, NULL, &hints, &result))
    {
        perror("getaddrinfo error");
        return -1;
    }

    if (result->ai_family == AF_INET)
        ((struct sockaddr_in *)result->ai_addr)->sin_port = htons(port);
    else if (result->ai_family == AF_INET6)
        ((struct sockaddr_in6 *)result->ai_addr)->sin6_port = htons(port);
    else
    {
        fprintf(stderr, "unknown ai_family %d", result->ai_family);
        freeaddrinfo(result);
        return -1;
    }
    memcpy(addr, result->ai_addr, result->ai_addrlen);
    *addrlen = result->ai_addrlen;

    if (-1 == (sock = socket(result->ai_family, SOCK_DGRAM, IPPROTO_UDP)))
    {
        perror("Cannot create socket");
        freeaddrinfo(result);
        return -1;
    }

#ifndef AS_CLIENT
    if (0 != bind(sock, result->ai_addr, result->ai_addrlen))
    {
        perror("Cannot bind");
        close(sock);
        freeaddrinfo(result);
        return -1;
    }
#endif

    freeaddrinfo(result);

    flags = fcntl(sock, F_GETFL, 0);
    if (flags != -1)
    {
        if (-1 != fcntl(sock, F_SETFL, flags | O_NONBLOCK))
            return sock;
    }
    perror("fcntl error");

    close(sock);
    return -1;
}

/*
 * Catch Ctrl-C and `kill`s, make sure route table gets cleaned before this process exit
 */
void cleanup(int signo)
{
    printf("Goodbye, cruel world....\n");
    if (signo == SIGHUP || signo == SIGINT || signo == SIGTERM)
    {
        cleanup_route_table();
        exit(0);
    }
}

void cleanup_when_sig_exit()
{
    struct sigaction sa;
    sa.sa_handler = &cleanup;
    sa.sa_flags = SA_RESTART;
    sigfillset(&sa.sa_mask);

    if (sigaction(SIGHUP, &sa, NULL) < 0)
    {
        perror("Cannot handle SIGHUP");
    }
    if (sigaction(SIGINT, &sa, NULL) < 0)
    {
        perror("Cannot handle SIGINT");
    }
    if (sigaction(SIGTERM, &sa, NULL) < 0)
    {
        perror("Cannot handle SIGTERM");
    }
}

uint8_t RandomByte()
{
    static bool random_inited = false;
    if (random_inited == false)
    {
        srand(time(nullptr));
        random_inited = true;
    }
    uint8_t r = rand() & 0xFF;
    return r;
}

int encrypt_inplace(char *pData, int len)
{    
    EncryptChain ec(len);
    ec.EncodeInplace((uint8_t*)pData, len);
    int swap_count = std::min(20, len / 3);
    for (int swap_index = 0; swap_index < swap_count; swap_index++)
    {
        uint8_t tmp = pData[swap_index];
        pData[swap_index] = pData[len - 1 - swap_index];
        pData[len - 1 - swap_index] = tmp;
    }
}

int decrypt_inplace(char *pData, int len)
{    
    int swap_count = std::min(20, len / 3);
    for (int swap_index = 0; swap_index < swap_count; swap_index++)
    {
        uint8_t tmp = pData[swap_index];
        pData[swap_index] = pData[len - 1 - swap_index];
        pData[len - 1 - swap_index] = tmp;
    }

    EncryptChain ec(len);
    ec.DecodeInplace((uint8_t*)pData, len);
}

bool get_ipv4_end_point_from_ipdata(const char *ipdata, int ipdata_len, uint32_t* ipv4_src, uint32_t* ipv4_des)
{
    if (ipdata_len < 20 + 4) // 20(ip header) 2(src port) 2(des port)
    {
        return false;
    }
    const struct iphdr* pHeader = (const struct iphdr*)ipdata;
    
    *ipv4_src = ntohl(pHeader->saddr);
    *ipv4_des = ntohl(pHeader->daddr);
    return true;
}

void print_errorno(const char* msg)
{
    perror(msg);
}



int main(int argc, char **argv)
{    
    int tun_fd;
    if ((tun_fd = tun_alloc()) < 0)
    {
        return 1;
    }

    ifconfig();
    cleanup_route_table();
    setup_route_table();
    cleanup_when_sig_exit();

    Clients clients;
    uint32_t ipv4_src;
    uint32_t ipv4_des;

    int udp_fd;
    struct sockaddr_storage udp_server_addr;
    socklen_t udp_server_addrlen = sizeof(udp_server_addr);

    if ((udp_fd = udp_bind((struct sockaddr *)&udp_server_addr, &udp_server_addrlen, PORT)) < 0)
    {
        return 1;
    }

    char readed_buf[READ_BUFFER_LEN];
    bzero(readed_buf, READ_BUFFER_LEN);    

    while (1)
    {
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(tun_fd, &readset);
        FD_SET(udp_fd, &readset);
        int max_fd = max(tun_fd, udp_fd) + 1;

        if (-1 == select(max_fd, &readset, NULL, NULL, NULL))
        {
            print_errorno("select error");
            break;
        }

        int r;
        if (FD_ISSET(tun_fd, &readset))
        {
            r = read(tun_fd, readed_buf, READ_BUFFER_LEN);
            if (r < 0)
            {
                // TODO: ignore some errno
                print_errorno("read from tun_fd error");
                break;
            }
            const struct iphead* pIpHead = (const struct iphead*)readed_buf;
            // only ipv4 is supported
            if (readed_buf[0] == 0x45 && get_ipv4_end_point_from_ipdata(readed_buf, r, &ipv4_src, &ipv4_des))
            {
                encrypt_inplace(readed_buf, r);
                
#if DEBUG_OUTPUT
                char src[16];
                char des[16];
                in_addr addr;
                addr.s_addr = htonl(ipv4_src);
                strcpy(src, inet_ntoa(addr));
                addr.s_addr = htonl(ipv4_des);
                strcpy(des, inet_ntoa(addr));
                printf("Writing to UDP %d bytes (%s)->(%s)...\n", r, src, des);
#endif
                ClientIpMap* clientIpMap = clients.get_client_by_ipv4(ipv4_des);
                if (clientIpMap != nullptr)
                {
                    const struct sockaddr* pSockAddr = (const struct sockaddr *)&(clientIpMap->GetSockAddrIn());
                    r = sendto(udp_fd, readed_buf, r, 0, pSockAddr, clientIpMap->GetSockAddrInLen());
                    if (r < 0)
                    {
                        // TODO: ignore some errno
                        print_errorno("sendto udp_fd error");
                        break;
                    }
                }
                else
                {                    
                    in_addr addr;
                    addr.s_addr = htonl(ipv4_des);
                    if (clientIpMap == nullptr)
                    {
                        printf("drop data to client ip via %s: %s\n", (clientIpMap == nullptr) ? "Disconnected" : "Speed", inet_ntoa(addr));
                    }
                }
            }
            else
            {
                uint8_t header_backup_received[5] = {0};
                memcpy(header_backup_received, readed_buf, sizeof(header_backup_received));
                printf("drop tun received (%02x-%02x-%02x-%02x-%02x)\n",
                        (int)header_backup_received[0],
                        (int)header_backup_received[1],
                        (int)header_backup_received[2],
                        (int)header_backup_received[3],
                        (int)header_backup_received[4]);
            }
        }

        if (FD_ISSET(udp_fd, &readset))
        {
            struct sockaddr_storage client_addr;
            socklen_t client_addrlen = sizeof(client_addr);

            r = recvfrom(udp_fd, readed_buf, READ_BUFFER_LEN, 0, (struct sockaddr *)&client_addr, &client_addrlen);
            if (r < 0)
            {
                print_errorno("recvfrom udp_fd error");
                break;
            }
            if (r < 20)
            {
                printf("recvfrom udp_fd error:len(%d)\n", r);
            }
            else
            {
                uint8_t header_backup_received[5] = {0};
                memcpy(header_backup_received, readed_buf, sizeof(header_backup_received));
                decrypt_inplace(readed_buf, r);
                uint8_t header_backup_decoded[5] = {0};
                memcpy(header_backup_decoded, readed_buf, sizeof(header_backup_decoded));

                bool droped = false;
                bool dropedViaSpeed = false;
                // only ipv4 supported
                if (r > 0 && ((readed_buf[0] & 0xF0) == 0x40) && get_ipv4_end_point_from_ipdata(readed_buf, r, &ipv4_src, &ipv4_des))
                {
                    ClientIpMap* clientIpMap = clients.set_client_by_ipv4(ipv4_src, &client_addr, client_addrlen);
#if DEBUG_OUTPUT
                    char src[16];
                    char des[16];
                    in_addr addr;
                    addr.s_addr = htonl(ipv4_src);
                    strcpy(src, inet_ntoa(addr));
                    addr.s_addr = htonl(ipv4_des);
                    strcpy(des, inet_ntoa(addr));
                    printf("Writing to tun %d bytes (%s)->(%s)...\n", r, src, des);
#endif
                    if (true)
                    {
                        r = write(tun_fd, readed_buf, r);
                        if (r < 0)
                        {
                            // TODO: ignore some errno
                            print_errorno("write tun_fd error");
                            break;
                        }
                    }
                    else
                    {
                        droped = true;
                        dropedViaSpeed = true;
                    }
                }
                else
                {
                    droped = true;
                }

                if (droped && (dropedViaSpeed == false))
                {
                    printf("drop udp received via %s (%02x-%02x-%02x-%02x-%02x) decoded(%02x-%02x-%02x-%02x-%02x)\n",
                            dropedViaSpeed ? "speed" : "invalided",
                           (int)header_backup_received[0],
                           (int)header_backup_received[1],
                           (int)header_backup_received[2],
                           (int)header_backup_received[3],
                           (int)header_backup_received[4],
                           (int)header_backup_decoded[0],
                           (int)header_backup_decoded[1],
                           (int)header_backup_decoded[2],
                           (int)header_backup_decoded[3],
                           (int)header_backup_decoded[4]);
                }
            }
        }
    }

    close(tun_fd);
    close(udp_fd);

    cleanup_route_table();
    return 0;
}
