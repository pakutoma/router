#pragma once
#include <net/ethernet.h>
int init_arp_table();
int register_arp_table(uint32_t ipaddr, uint8_t macaddr[ETH_ALEN]);
int find_macaddr(uint32_t ipaddr, uint8_t macaddr[ETH_ALEN]);
void print_arp_table();