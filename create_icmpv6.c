#include "checksum.h"
#include "ether_frame.h"
#include "log.h"
#include "settings.h"
#include <arpa/inet.h>
#include <netinet/icmp6.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <stdlib.h>

#ifndef IPV6_MMTU
#define IPV6_MMTU 1280
#endif

static int token_bucket;

void icmpv6_token_bucket_init() {
    token_bucket = get_ratelimit_token_per_second();
}

void icmpv6_token_bucket_add_token() {
    token_bucket = get_ratelimit_token_per_second();
}

bool icmpv6_token_bucket_consume_token() {
    if (token_bucket == 0) {
        return false;
    }
    token_bucket--;
    return true;
}

ether_frame_t *create_icmpv6_error(ether_frame_t *received_frame, uint8_t type, uint8_t code, uint32_t data) {
    if (!icmpv6_token_bucket_consume_token()) {
        return NULL;
    }

    ether_frame_t *icmp6_frame;

    if ((icmp6_frame = calloc(1, sizeof(ether_frame_t))) == NULL) {
        log_perror("calloc");
        return NULL;
    }

    int header_size = sizeof(struct ip6_hdr) + sizeof(struct icmp6_hdr);
    int option_size = received_frame->payload_size + header_size > IPV6_MMTU ? IPV6_MMTU - header_size : received_frame->payload_size;

    icmp6_frame->payload_size = header_size + option_size;
    if ((icmp6_frame->payload = calloc(1, icmp6_frame->payload_size)) == NULL) {
        log_perror("calloc");
        return NULL;
    }

    device_t *device = get_device(received_frame->index);

    memcpy(icmp6_frame->header.ether_shost, device->hw_addr, sizeof(uint8_t) * ETH_ALEN);
    memcpy(icmp6_frame->header.ether_dhost, received_frame->header.ether_shost, sizeof(uint8_t) * ETH_ALEN);

    icmp6_frame->header.ether_type = htons(ETHERTYPE_IPV6);

    struct ip6_hdr *ip6_header = (struct ip6_hdr *)icmp6_frame->payload;

    ip6_header->ip6_flow = 0;
    ip6_header->ip6_nxt = IPPROTO_ICMPV6;
    ip6_header->ip6_plen = htons(sizeof(struct icmp6_hdr) + option_size);
    ip6_header->ip6_hlim = 0xFF;
    ip6_header->ip6_vfc = 6 << 4 | 0;

    memcpy(ip6_header->ip6_src.s6_addr, device->addr6_list[0].s6_addr, sizeof(uint8_t) * INET_ADDRSTRLEN);
    memcpy(ip6_header->ip6_dst.s6_addr, ((struct ip6_hdr *)received_frame->payload)->ip6_src.s6_addr, sizeof(uint8_t) * INET_ADDRSTRLEN);

    struct icmp6_hdr *icmp6_header = (struct icmp6_hdr *)(ip6_header + 1);
    icmp6_header->icmp6_type = type;
    icmp6_header->icmp6_code = code;
    icmp6_header->icmp6_data32[0] = htonl(data);

    uint8_t *option = (uint8_t *)(icmp6_header + 1);
    memcpy(option, received_frame->payload, option_size);
    icmp6_header->icmp6_cksum = calc_icmp6_checksum(ip6_header);
    return icmp6_frame;
}
