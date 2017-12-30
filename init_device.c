#include "log.h"
#include "settings.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/if.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netpacket/packet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
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
    log_stdout("%s OK\n", device->netif_name);
    log_stdout("addr=%s\n", inet_ntoa(device->addr));
    log_stdout("subnet=%s\n", inet_ntoa(device->subnet));
    log_stdout("netmask=%s\n", inet_ntoa(device->netmask));

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
    struct sockaddr_in addr = {0};
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

    if (ioctl(sock_desc, SIOCGIFADDR, &ifreq) == -1) {
        log_perror("ioctl");
        close(sock_desc);
        return -1;
    } else if (ifreq.ifr_addr.sa_family != PF_INET) {
        log_error("%s not PF_INET\n", device->netif_name);
        log_error("%d\n", ifreq.ifr_addr.sa_family);
        close(sock_desc);
        return -1;
    } else {
        memcpy(&addr, &ifreq.ifr_addr, sizeof(struct sockaddr_in));
        device->addr = addr.sin_addr;
    }

    if (ioctl(sock_desc, SIOCGIFNETMASK, &ifreq) == -1) {
        log_perror("ioctl");
        close(sock_desc);
        return -1;
    } else {
        memcpy(&addr, &ifreq.ifr_addr, sizeof(struct sockaddr_in));
        device->netmask = addr.sin_addr;
    }

    device->subnet.s_addr = ((device->addr.s_addr) & (device->netmask.s_addr));

    close(sock_desc);
    return 0;
}
