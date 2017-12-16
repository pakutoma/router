#include "ether_frame.h"
ether_frame_t *create_arp_request(uint8_t own_macaddr[ETH_ALEN], uint32_t own_ipaddr, uint32_t neighbor_ipaddr);