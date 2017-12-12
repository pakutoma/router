#pragma once
#include <net/ethernet.h>
int init_arp_table();
int register_arp_table(unsigned int ipaddr, unsigned char macaddr[ETH_ALEN]);
int find_macaddr(unsigned int ipaddr, unsigned char macaddr[ETH_ALEN]);