#include "checksum.h"
#include "ether_frame.h"
#include "log.h"
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <string.h>
ether_frame_t *create_time_exceeded_request(uint8_t src_macaddr[8], uint8_t dst_macaddr[8], uint32_t src_ipaddr, uint32_t dst_ipaddr, uint8_t packet_data[64]) {
    ether_frame_t *icmp_frame;
    struct iphdr *ip_header;
    struct icmp *icmp_header;

    if ((icmp_frame = malloc(sizeof(ether_frame_t))) == NULL) {
        log_perror("malloc");
        return -1;
    }
    if ((icmp_frame->payload = malloc(sizeof(struct iphdr) + sizeof(struct icmp) + sizeof(uint8_t) * 64)) == NULL) {
        log_perror("malloc");
        return -1;
    }
    icmp_frame->payload_size = sizeof(struct iphdr) + sizeof(struct icmp) + sizeof(uint8_t) * 64;
    memcpy(icmp_frame->header.ether_shost, src_macaddr, sizeof(uint8_t) * ETH_ALEN);
    memset(icmp_frame->header.ether_dhost, dst_macaddr, sizeof(uint8_t) * ETH_ALEN);
    icmp_frame->header.ether_type = htons(ETHERTYPE_IP);
    ip_header = (struct iphdr *)icmp_frame->payload;
    ip_header->version = 4;
    ip_header->ihl = 20 / 4;
    ip_header->tos = 0;
    ip_header->tot_len = htons(sizeof(struct icmp) + 64 * sizeof(uint8_t));
    ip_header->id = 0;
    ip_header->frag_off = 0;
    ip_header->ttl = 64;
    ip_header->protocol = IPPROTO_ICMP;
    ip_header->check = 0;
    ip_header->saddr = src_ipaddr;
    ip_header->daddr = dst_ipaddr;
    ip_header->check = calc_checksum((uint8_t *)ip_header, sizeof(struct iphdr));

    icmp_header = ip_header + sizeof(struct iphdr);
    icmp_header->icmp_type = ICMP_TIME_EXCEEDED;
    icmp_header->icmp_code = ICMP_TIMXCEED_INTRANS;
    icmp_header->icmp_cksum = 0;
    icmp_header->icmp_void = 0;
    icmp_header->icmp_cksum = 0;

    memcpy(icmp_header + sizeof(struct icmp), packet_data, sizeof(uint8_t) * 64);

    icmp_header->icmp_cksum = calc_checksum(icmp_header, sizeof(struct icmp) + sizeof(uint8_t) * 64);

    return icmp_frame;
}