#pragma once
#include "ether_frame.h"
#include <net/ethernet.h>
int init_arp_waiting_list();
int add_arp_waiting_list(ether_frame_t *ether_frame, uint32_t neighbor_ipaddr);
void send_waiting_ethernet_frame_matching_ipaddr(uint32_t ipaddr, uint8_t macaddr[ETH_ALEN]);
void print_waiting_list();