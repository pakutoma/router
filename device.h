#pragma once
#include <net/ethernet.h>
#include <netinet/in.h>

#define NUMBER_OF_DEVICES 2
#define GATEWAY_DEVICE 1
typedef struct {
    char *netif_name;
    int sock_desc;
    unsigned char hw_addr[ETH_ALEN];
    struct in_addr addr, subnet, netmask;
} device_t;

int find_device(uint8_t macaddr[ETH_ALEN], device_t devices[NUMBER_OF_DEVICES]);