#pragma once
#include "ether_frame.h"
#include <net/ethernet.h>
int init_arp_waiting_list();
int add_arp_waiting_list(ether_frame_t *ether_frame, unsigned int neighbor_ipaddr);
void send_waiting_ethernet_frame_matching_ipaddr(unsigned int ipaddr, unsigned char macaddr[ETH_ALEN]);