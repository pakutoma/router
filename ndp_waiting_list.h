#pragma once
#include "ether_frame.h"
int init_ndp_waiting_list();
int add_ndp_waiting_list(ether_frame_t *ether_frame, struct in6_addr *neighbor_ipaddr);
void send_waiting_frame(struct in6_addr *ipaddr, struct ether_addr *macaddr);
