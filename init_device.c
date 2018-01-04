#include "log.h"
#include "settings.h"
#include <arpa/inet.h>
#include <asm/types.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <linux/if.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netpacket/packet.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int init_raw_socket(char *device_name);
int get_device_info(device_t *device);

int init_device(device_t *device) {
    if (get_device_info(device) == -1) {
        return -1;
    }
    if ((device->sock_desc = init_raw_socket(device->netif_name)) == -1) {
        return -1;
    }
    int fd_flgs = fcntl(device->sock_desc, F_GETFD);
    fcntl(device->sock_desc, F_SETFD, fd_flgs | O_NONBLOCK);
    char buf[64];
    log_stdout("%s OK\n", device->netif_name);
    log_stdout("addr=%s\n", inet_ntop(AF_INET, &device->addr, buf, 64));
    log_stdout("subnet=%s\n", inet_ntop(AF_INET, &device->subnet, buf, 64));
    log_stdout("netmask=%s\n", inet_ntop(AF_INET, &device->netmask, buf, 64));
    for (int i = 0; i < device->addr6_list_length; i++) {
        log_stdout("addr6[%d]=%s\n", i, inet_ntop(AF_INET6, &device->addr6_list[i], buf, 64));
        log_stdout("subnet6[%d]=%s\n", i, inet_ntop(AF_INET6, &device->subnet6_list[i], buf, 64));
        log_stdout("netmask6[%d]=%s\n", i, inet_ntop(AF_INET6, &device->netmask6_list[i], buf, 64));
    }

    return 0;
}

int device_fin(device_t *device) {
    close(device->sock_desc);
    return 0;
}

int init_raw_socket(char *device_name) {
    int sock_desc;
    struct ifreq ifreq = {{{0}}};
    struct sockaddr_ll sa = {0};
    if ((sock_desc = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        log_perror("socket");
        return -1;
    }

    strncpy(ifreq.ifr_name, device_name, sizeof(ifreq.ifr_name) - 1);
    if (ioctl(sock_desc, SIOCGIFINDEX, &ifreq) < 0) {
        log_perror("ioctl");
        close(sock_desc);
        return -1;
    }

    sa.sll_family = PF_PACKET;
    sa.sll_protocol = htons(ETH_P_ALL);
    sa.sll_ifindex = ifreq.ifr_ifindex;
    if (bind(sock_desc, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        log_perror("bind");
        close(sock_desc);
        return -1;
    }

    return sock_desc;
}

int get_device_info(device_t *device) {
    struct ifreq ifreq = {{{0}}};
    int sock_desc;
    uint8_t *ptr;

    if ((sock_desc = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
        log_perror("socket");
        return -1;
    }

    strncpy(ifreq.ifr_name, device->netif_name, sizeof(ifreq.ifr_name) - 1);

    if (ioctl(sock_desc, SIOCGIFHWADDR, &ifreq) == -1) {
        log_perror("ioctl");
        close(sock_desc);
        return -1;
    } else {
        ptr = (uint8_t *)&ifreq.ifr_hwaddr.sa_data;
        memcpy(device->hw_addr, ptr, 6);
    }

    close(sock_desc);

    struct ifaddrs *ifa;
    getifaddrs(&ifa);
    struct ifaddrs *ifa_p = ifa;
    device->addr6_list_length = 0;
    while (ifa_p != NULL) {
        if (strcmp(ifa_p->ifa_name, device->netif_name) == 0 && ifa_p->ifa_addr->sa_family == AF_INET6) {
            device->addr6_list_length++;
        }
        ifa_p = ifa_p->ifa_next;
    }
    if ((device->addr6_list = calloc(device->addr6_list_length, sizeof(struct in6_addr))) == NULL) {
        log_perror("calloc");
        return -1;
    }
    if ((device->subnet6_list = calloc(device->addr6_list_length, sizeof(struct in6_addr))) == NULL) {
        free(device->addr6_list);
        log_perror("calloc");
        return -1;
    }
    if ((device->netmask6_list = calloc(device->addr6_list_length, sizeof(struct in6_addr))) == NULL) {
        free(device->addr6_list);
        free(device->subnet6_list);
        log_perror("calloc");
        return -1;
    }

    ifa_p = ifa;
    int index = 0;
    while (ifa_p != NULL) {
        if (strcmp(ifa_p->ifa_name, device->netif_name) == 0 && ifa_p->ifa_addr->sa_family != AF_PACKET) {
            if (ifa_p->ifa_addr->sa_family == AF_INET) {
                device->addr = ((struct sockaddr_in *)ifa_p->ifa_addr)->sin_addr;
                device->netmask = ((struct sockaddr_in *)ifa_p->ifa_netmask)->sin_addr;
                device->subnet.s_addr = device->addr.s_addr & device->netmask.s_addr;
            } else {
                device->addr6_list[index] = ((struct sockaddr_in6 *)ifa_p->ifa_addr)->sin6_addr;
                device->netmask6_list[index] = ((struct sockaddr_in6 *)ifa_p->ifa_netmask)->sin6_addr;
                for (int i = 0; i < 16; i++) {
                    device->subnet6_list[index].s6_addr[i] = device->addr6_list[index].s6_addr[i] & device->netmask6_list[index].s6_addr[i];
                }
                index++;
            }
        }
        ifa_p = ifa_p->ifa_next;
    }
    freeifaddrs(ifa);

    return 0;
}
