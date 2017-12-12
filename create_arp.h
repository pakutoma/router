#include "ether_frame.h"
ether_frame_t *create_arp_request(ether_frame_t *arrived_frame, uint32_t own_ipaddr, uint32_t neighbor_ipaddr);