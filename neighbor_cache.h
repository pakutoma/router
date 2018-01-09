#pragma once
#include <stdbool.h>
#include "ether_frame.h"

bool query_macaddr(struct in6_addr *ipaddr, struct ether_addr *macaddr);
int init_neighbor_cache();
void analyze_ndp_packet(ether_frame_t *ndp_frame);
void update_status();
