#include "ether_frame.h"
#include "log.h"
#include "ndp_waiting_list.h"
#include "neighbor_cache.h"
#include "process_ndp_packet.h"
#include "send_queue.h"
#include <netinet/icmp6.h>
#include <netinet/ip6.h>

#define TYPE_ICMPV6 58
int decrement_hop_limit(struct ip6_hdr *header);
int send_v6packet(ether_frame_t *ether_frame, struct ether_addr *src_macaddr, struct in6_addr *origin_device_ipaddr, struct in6_addr *neighbor_ipaddr);

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

    if (decrement_hop_limit(ip6_header) == 0) {
        log_stdout("IP packet is time exceeded.\n");
        //TODO: send ICMPv6 time exceed
        return -1;
    }

    device_t *out_device;
    if ((out_device = find_device_by_ipv6addr(&ip6_header->ip6_dst)) == NULL) {
        struct in6_addr next_router_ipaddr = get_next_router_ipv6_addr();
        if (send_v6packet(ether_frame, (struct ether_addr *)get_device(get_gateway_device_index())->hw_addr, &(get_device(get_gateway_device_index())->addr6_list[0]), &next_router_ipaddr) == -1) {
            return -1;
        }
    } else {
        if (send_v6packet(ether_frame, (struct ether_addr *)out_device->hw_addr, &out_device->addr6_list[0], &ip6_header->ip6_dst) == -1) {
            return -1;
        }
    }
    return 0;
}

int send_v6packet(ether_frame_t *ether_frame, struct ether_addr *src_macaddr, struct in6_addr *origin_device_ipaddr, struct in6_addr *neighbor_ipaddr) {
    struct ether_addr dst_macaddr;
    if (query_macaddr(neighbor_ipaddr, &dst_macaddr)) {
        memcpy(ether_frame->header.ether_dhost, dst_macaddr.ether_addr_octet, sizeof(uint8_t) * ETH_ALEN);
        memcpy(ether_frame->header.ether_shost, src_macaddr->ether_addr_octet, sizeof(uint8_t) * ETH_ALEN);
        enqueue_send_queue(ether_frame);
    } else {
        memcpy(ether_frame->header.ether_shost, src_macaddr->ether_addr_octet, sizeof(uint8_t) * ETH_ALEN);
        add_ndp_waiting_list(ether_frame, neighbor_ipaddr);
    }
    return 0;
}

int decrement_hop_limit(struct ip6_hdr *header) {
    int hlim = header->ip6_hlim - 1;
    if (hlim == 0) {
        return 0;
    }
    header->ip6_hlim = hlim;
    return hlim;
}
