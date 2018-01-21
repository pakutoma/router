#include "create_ndp.h"
#include "ether_frame.h"
#include "log.h"
#include "neighbor_cache.h"
#include "send_queue.h"
#include <netinet/icmp6.h>
#include <netinet/ip6.h>

int process_ndp_packet(ether_frame_t *ether_frame) {
    struct ip6_hdr *ip6_header = (struct ip6_hdr *)ether_frame->payload;
    struct icmp6_hdr *icmp6_header = (struct icmp6_hdr *)(ip6_header + 1);
    if (icmp6_header->icmp6_type == ND_ROUTER_SOLICIT) {
        ether_frame_t *ra_frame;
        if ((ra_frame = create_router_advertisement(ether_frame->index, true, &ip6_header->ip6_src, ether_frame->header.ether_shost)) == NULL) {
            return -1;
        }
        enqueue_send_queue(ra_frame);
    }
    analyze_ndp_packet(ether_frame);
    return 0;
}
