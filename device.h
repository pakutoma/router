#pragma once
#include <net/ethernet.h>
#include <netinet/in.h>

#define NUMBER_OF_DEVICES 2
#define GATEWAY_DEVICE 1
typedef struct {
    char *netif_name;
    int sock_desc;
    uint8_t hw_addr[ETH_ALEN];
    struct in_addr addr, subnet, netmask;
} device_t;

device_t *find_device_by_macaddr(uint8_t macaddr[ETH_ALEN]);
device_t *find_device_by_ipaddr(uint32_t ipaddr);
int find_device_index_by_macaddr(uint8_t macaddr[ETH_ALEN]);
int find_device_index_by_sock_desc(int sock_desc);
int device_init(char *device_names[NUMBER_OF_DEVICES]);
device_t *get_device(int index);
