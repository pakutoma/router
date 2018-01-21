#include "ether_frame.h"
#include <netinet/if_ether.h>
#include <netinet/in.h>

#define MAX_INITIAL_RTR_ADVERT_INTERVAL 16
#define MAX_INITIAL_RTR_ADVERTISEMENTS 3
#define MAX_FINAL_RTR_ADVERTISEMENTS 3
#define MIN_DELAY_BETWEEN_RAS 3
#define MAX_RA_DELAY_TIME 0.5

ether_frame_t *create_router_advertisement(int device_index, bool is_reply, struct in6_addr *ipaddr, uint8_t macaddr[ETH_ALEN]);
ether_frame_t *create_neighbor_solicitation(struct in6_addr *ipaddr, bool is_address_resolution);
