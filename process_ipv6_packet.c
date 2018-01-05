#include "ether_frame.h"
#include "log.h"
#include "process_ndp_packet.h"
#include <netinet/icmp6.h>
#include <netinet/ip6.h>

#define TYPE_ICMPV6 58

int process_ipv6_packet(ether_frame_t *ether_frame) {

    if (ether_frame->payload_size < sizeof(struct ip6_hdr)) {
        log_stdout("IPv6 packet is too short.\n");
        return -1;
    }

    if (!ether_frame->has_index) {
        log_stdout("IPv6 packet don't have index.\n");
        return -1;
    }

    struct ip6_hdr *ip6_header = (struct ip6_hdr *)ether_frame->payload;
    if (ip6_header->ip6_nxt == TYPE_ICMPV6) {
        struct icmp6_hdr *icmp6_header = (struct icmp6_hdr *)(ip6_header + 1);
        if (icmp6_header->icmp6_type >= ND_ROUTER_SOLICIT && icmp6_header->icmp6_type <= ND_NEIGHBOR_ADVERT) {
            return process_ndp_packet(ether_frame);
        }
    }
    return -1;
}
