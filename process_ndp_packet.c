#include "create_ndp.h"
#include "ether_frame.h"
#include "log.h"
#include "neighbor_cache.h"
#include "send_queue.h"
#include <netinet/icmp6.h>
#include <netinet/ip6.h>

bool exists_target_ipv6addr(int index, struct in6_addr *ns_header);

int process_ndp_packet(ether_frame_t *ether_frame) {
    struct ip6_hdr *ip6_header = (struct ip6_hdr *)ether_frame->payload;
    struct icmp6_hdr *icmp6_header = (struct icmp6_hdr *)(ip6_header + 1);
    if (icmp6_header->icmp6_type == ND_ROUTER_SOLICIT) {
        ether_frame_t *ra_frame;
        if ((ra_frame = create_router_advertisement(ether_frame->index, true, &ip6_header->ip6_src, ether_frame->header.ether_shost)) == NULL) {
            return -1;
        }
        enqueue_send_queue(ra_frame);
    } else if (icmp6_header->icmp6_type == ND_NEIGHBOR_SOLICIT) {
        struct nd_neighbor_solicit *nd_header = (struct nd_neighbor_solicit *)(ip6_header + 1);
        if (exists_target_ipv6addr(ether_frame->index, &nd_header->nd_ns_target)) {
            ether_frame_t *na_frame;
            if ((na_frame = create_neighbor_advertisement(ether_frame->index, true, &nd_header->nd_ns_target, &ip6_header->ip6_src, (struct ether_addr *)ether_frame->header.ether_shost)) == NULL) {
                return -1;
            }
            enqueue_send_queue(na_frame);
        }
    }
    analyze_ndp_packet(ether_frame);
    return 0;
}

bool exists_target_ipv6addr(int index, struct in6_addr *target_addr) {
    device_t *device = get_device(index);
    for (size_t i = 0; i < device->addr6_list_length; i++) {
        if (memcmp(target_addr->s6_addr, device->addr6_list[i].s6_addr, INET_ADDRSTRLEN) == 0) {
            return true;
        }
    }
    return false;
}
