#include "checksum.h"
#include "ether_frame.h"
#include "log.h"
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/icmp6.h>
#include <netinet/if_ether.h>
#include <netinet/ip6.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ether_frame_t *create_router_advertisement(int device_index, bool is_reply, struct in6_addr *ipaddr, uint8_t macaddr[ETH_ALEN]) {
    ether_frame_t *icmp6_frame;

    if ((icmp6_frame = calloc(1, sizeof(ether_frame_t))) == NULL) {
        log_perror("calloc");
        return NULL;
    }

    device_t *device = get_device(device_index);
    adv_settings_t *adv_settings = &device->adv_settings;

    int header_option_size = sizeof(struct nd_opt_hdr) + sizeof(uint8_t) * ETH_ALEN;
    header_option_size += sizeof(struct nd_opt_prefix_info) * adv_settings->adv_prefix_list_length;
    header_option_size += adv_settings->adv_link_mtu == 0 ? 0 : sizeof(struct nd_opt_mtu);
    if (adv_settings->adv_recursive_dns_server_list_length != 0) {
        header_option_size += sizeof(uint8_t) * 8 + adv_settings->adv_recursive_dns_server_list_length * 16 * sizeof(uint8_t);
    }
    uint32_t dnssl_length_octet = 0;
    if (adv_settings->adv_dns_search_list_length != 0) {
        for (int i = 0; i < adv_settings->adv_dns_search_list_length; i++) {
            dnssl_length_octet += strlen(adv_settings->adv_dns_search_list[i]) + 1;
        }
        if (dnssl_length_octet % 8 != 0) {
            dnssl_length_octet += 8 - dnssl_length_octet % 8;
        }
        header_option_size += 8 * sizeof(uint8_t) + dnssl_length_octet;
    }

    if ((icmp6_frame->payload = calloc(1, sizeof(struct ip6_hdr) + sizeof(struct nd_router_advert) + header_option_size)) == NULL) {
        log_perror("calloc");
        return NULL;
    }
    icmp6_frame->payload_size = sizeof(struct ip6_hdr) + sizeof(struct nd_router_advert) + header_option_size;

    memcpy(icmp6_frame->header.ether_shost, device->hw_addr, sizeof(uint8_t) * ETH_ALEN);
    if (is_reply) {
        memcpy(icmp6_frame->header.ether_dhost, macaddr, sizeof(uint8_t) * ETH_ALEN);
    } else {
        icmp6_frame->header.ether_dhost[0] = 0x33;
        icmp6_frame->header.ether_dhost[1] = 0x33;
        icmp6_frame->header.ether_dhost[2] = 0x00;
        icmp6_frame->header.ether_dhost[3] = 0x00;
        icmp6_frame->header.ether_dhost[4] = 0x00;
        icmp6_frame->header.ether_dhost[5] = 0x01;
    }

    icmp6_frame->header.ether_type = htons(ETHERTYPE_IPV6);

    struct ip6_hdr *ip6_header;
    ip6_header = (struct ip6_hdr *)icmp6_frame->payload;

    ip6_header->ip6_flow = 0;
    ip6_header->ip6_nxt = IPPROTO_ICMPV6;
    ip6_header->ip6_plen = htons(sizeof(struct nd_router_advert) + header_option_size);
    ip6_header->ip6_hlim = 0xFF;
    ip6_header->ip6_vfc = 6 << 4 | 0;

    for (size_t i = 0; i < device->addr6_list_length; i++) {
        if (device->addr6_list[i].s6_addr[0] == 0xfe && device->addr6_list[i].s6_addr[1] & 0x80) {
            ip6_header->ip6_src = device->addr6_list[i];
        }
    }

    if (is_reply) {
        ip6_header->ip6_dst = *ipaddr;
    } else {
        inet_pton(AF_INET6, "ff02::1", &ip6_header->ip6_dst);
    }

    struct nd_router_advert *ra_header;
    ra_header = (struct nd_router_advert *)(ip6_header + 1);
    ra_header->nd_ra_type = ND_ROUTER_ADVERT;
    ra_header->nd_ra_code = 0;
    ra_header->nd_ra_curhoplimit = adv_settings->adv_cur_hop_limit;
    ra_header->nd_ra_router_lifetime = htons(adv_settings->adv_default_lifetime);
    ra_header->nd_ra_flags_reserved = 0;
    ra_header->nd_ra_cksum = 0;
    ra_header->nd_ra_reachable = htonl(adv_settings->adv_reachable_time);
    ra_header->nd_ra_retransmit = htonl(adv_settings->adv_retrans_timer);

    struct nd_opt_hdr *linklayer_header = (struct nd_opt_hdr *)(ra_header + 1);
    linklayer_header->nd_opt_type = ND_OPT_SOURCE_LINKADDR;
    linklayer_header->nd_opt_len = 1;
    uint8_t *linklayer_address = (uint8_t *)(linklayer_header + 1);
    memcpy(linklayer_address, device->hw_addr, sizeof(uint8_t) * ETH_ALEN);

    struct nd_opt_prefix_info *prefix_option = (struct nd_opt_prefix_info *)(linklayer_address + ETH_ALEN);
    for (int i = 0; i < adv_settings->adv_prefix_list_length; i++) {
        prefix_option->nd_opt_pi_type = ND_OPT_PREFIX_INFORMATION;
        prefix_option->nd_opt_pi_len = 4;
        prefix_option->nd_opt_pi_prefix_len = adv_settings->adv_prefix_list[i].adv_address_prefix_length;
        prefix_option->nd_opt_pi_flags_reserved = ND_OPT_PI_FLAG_ONLINK | ND_OPT_PI_FLAG_AUTO;
        prefix_option->nd_opt_pi_valid_time = htonl(adv_settings->adv_prefix_list[i].adv_valid_life_time);
        prefix_option->nd_opt_pi_preferred_time = htonl(adv_settings->adv_prefix_list[i].adv_preferred_life_time);
        prefix_option->nd_opt_pi_prefix = adv_settings->adv_prefix_list[i].adv_address_prefix;
        prefix_option++;
    }

    struct nd_opt_mtu *mtu_option = (struct nd_opt_mtu *)prefix_option;
    if (adv_settings->adv_link_mtu != 0) {
        mtu_option->nd_opt_mtu_type = ND_OPT_MTU;
        mtu_option->nd_opt_mtu_len = 1;
        mtu_option->nd_opt_mtu_type = htonl(adv_settings->adv_link_mtu);
        mtu_option++;
    }

    uint8_t *rdnss_header = (uint8_t *)mtu_option;
    if (adv_settings->adv_recursive_dns_server_list_length > 0) {
        struct nd_opt_hdr *rdnss_type_and_len = (struct nd_opt_hdr *)rdnss_header;
        const int nd_opt_rdnss = 25;
        rdnss_type_and_len->nd_opt_type = nd_opt_rdnss;
        rdnss_type_and_len->nd_opt_len = 1 + adv_settings->adv_recursive_dns_server_list_length * 2;
        rdnss_type_and_len++;
        uint16_t *rdnss_reserved = (uint16_t *)rdnss_type_and_len;
        *rdnss_reserved = 0;
        rdnss_reserved++;
        uint32_t *rdnss_lifetime = (uint32_t *)rdnss_reserved;
        *rdnss_lifetime = htonl((uint32_t)adv_settings->adv_default_lifetime);
        rdnss_lifetime++;
        struct in6_addr *rdnss_addresses = (struct in6_addr *)rdnss_lifetime;
        for (int i = 0; i < adv_settings->adv_recursive_dns_server_list_length; i++) {
            rdnss_addresses[i] = adv_settings->adv_recursive_dns_server_list[i];
        }
        rdnss_header += 8 * sizeof(uint8_t) + adv_settings->adv_recursive_dns_server_list_length * sizeof(struct in6_addr);
    }

    uint8_t *dnssl_header = (uint8_t *)rdnss_header;
    if (adv_settings->adv_dns_search_list_length > 0) {
        struct nd_opt_hdr *dnssl_type_and_len = (struct nd_opt_hdr *)dnssl_header;
        const int nd_opt_dnssl = 31;
        dnssl_type_and_len->nd_opt_type = nd_opt_dnssl;
        dnssl_type_and_len->nd_opt_len = 1 + dnssl_length_octet / 8;
        dnssl_type_and_len++;
        uint16_t *dnssl_reserved = (uint16_t *)dnssl_type_and_len;
        *dnssl_reserved = 0;
        dnssl_reserved++;
        uint32_t *dnssl_lifetime = (uint32_t *)dnssl_reserved;
        *dnssl_lifetime = htonl((uint32_t)adv_settings->adv_default_lifetime);
        dnssl_lifetime++;
        char *dnssl_domains = (char *)dnssl_lifetime;
        for (int i = 0; i < adv_settings->adv_dns_search_list_length; i++) {
            strcpy(dnssl_domains, adv_settings->adv_dns_search_list[i]);
            dnssl_domains += strlen(adv_settings->adv_dns_search_list[i]) + 1;
        }
        dnssl_header += 8 * sizeof(uint8_t) + dnssl_length_octet;
    }

    ra_header->nd_ra_cksum = calc_icmp6_checksum(ip6_header);
    return icmp6_frame;
}

ether_frame_t *create_neighbor_solicitation(struct in6_addr *ipaddr, bool is_address_resolution) {
    ether_frame_t *icmp6_frame;

    if ((icmp6_frame = calloc(1, sizeof(ether_frame_t))) == NULL) {
        log_perror("calloc");
        return NULL;
    }

    device_t *device = find_device_by_ipv6addr(ipaddr);

    int header_option_size = sizeof(struct nd_opt_hdr) + sizeof(uint8_t) * ETH_ALEN;

    if ((icmp6_frame->payload = calloc(1, sizeof(struct ip6_hdr) + sizeof(struct nd_neighbor_solicit) + header_option_size)) == NULL) {
        log_perror("calloc");
        return NULL;
    }
    icmp6_frame->payload_size = sizeof(struct ip6_hdr) + sizeof(struct nd_neighbor_solicit) + header_option_size;

    memcpy(icmp6_frame->header.ether_shost, device->hw_addr, sizeof(uint8_t) * ETH_ALEN);
    icmp6_frame->header.ether_dhost[0] = 0x33;
    icmp6_frame->header.ether_dhost[1] = 0x33;
    icmp6_frame->header.ether_dhost[2] = 0x00;
    icmp6_frame->header.ether_dhost[3] = 0x00;
    icmp6_frame->header.ether_dhost[4] = 0x00;
    icmp6_frame->header.ether_dhost[5] = 0x01;

    icmp6_frame->header.ether_type = htons(ETHERTYPE_IPV6);

    struct ip6_hdr *ip6_header;
    ip6_header = (struct ip6_hdr *)icmp6_frame->payload;

    ip6_header->ip6_flow = 0;
    ip6_header->ip6_nxt = IPPROTO_ICMPV6;
    ip6_header->ip6_plen = htons(sizeof(struct nd_neighbor_solicit) + header_option_size);
    ip6_header->ip6_hlim = 0xFF;
    ip6_header->ip6_vfc = 6 << 4 | 0;

    ip6_header->ip6_src = device->addr6_list[0];
    if (is_address_resolution) {
        inet_pton(AF_INET6, "ff02::1:ffff:ffff", &ip6_header->ip6_dst);
        ip6_header->ip6_dst.s6_addr[13] = ipaddr->s6_addr[13];
        ip6_header->ip6_dst.s6_addr[14] = ipaddr->s6_addr[14];
        ip6_header->ip6_dst.s6_addr[15] = ipaddr->s6_addr[15];
    } else {
        ip6_header->ip6_dst = *ipaddr;
    }

    struct nd_neighbor_solicit *ns_header;
    ns_header = (struct nd_neighbor_solicit *)(ip6_header + 1);
    ns_header->nd_ns_type = ND_NEIGHBOR_SOLICIT;
    ns_header->nd_ns_code = 0;
    ns_header->nd_ns_reserved = 0;
    ns_header->nd_ns_target = *ipaddr;

    struct nd_opt_hdr *linklayer_header = (struct nd_opt_hdr *)(ns_header + 1);
    linklayer_header->nd_opt_type = ND_OPT_SOURCE_LINKADDR;
    linklayer_header->nd_opt_len = 1;
    uint8_t *linklayer_address = (uint8_t *)(linklayer_header + 1);
    memcpy(linklayer_address, device->hw_addr, sizeof(uint8_t) * ETH_ALEN);

    ns_header->nd_ns_cksum = calc_icmp6_checksum(ip6_header);
    return icmp6_frame;
}

ether_frame_t *create_neighbor_advertisement(int device_index, bool is_reply, struct in6_addr *target_ipaddr, struct in6_addr *ipaddr, struct ether_addr *macaddr) {
    ether_frame_t *icmp6_frame;

    if ((icmp6_frame = calloc(1, sizeof(ether_frame_t))) == NULL) {
        log_perror("calloc");
        return NULL;
    }

    device_t *device = get_device(device_index);

    int header_option_size = sizeof(struct nd_opt_hdr) + sizeof(struct ether_addr);

    icmp6_frame->payload_size = sizeof(struct ip6_hdr) + sizeof(struct nd_neighbor_advert) + header_option_size;
    if ((icmp6_frame->payload = calloc(1, icmp6_frame->payload_size)) == NULL) {
        log_perror("calloc");
        return NULL;
    }

    memcpy(icmp6_frame->header.ether_shost, device->hw_addr, sizeof(uint8_t) * ETH_ALEN);
    if (is_reply) {
        memcpy(icmp6_frame->header.ether_dhost, macaddr->ether_addr_octet, ETH_ALEN);
    } else {
        icmp6_frame->header.ether_dhost[0] = 0x33;
        icmp6_frame->header.ether_dhost[1] = 0x33;
        icmp6_frame->header.ether_dhost[2] = 0x00;
        icmp6_frame->header.ether_dhost[3] = 0x00;
        icmp6_frame->header.ether_dhost[4] = 0x00;
        icmp6_frame->header.ether_dhost[5] = 0x01;
    }

    icmp6_frame->header.ether_type = htons(ETHERTYPE_IPV6);

    struct ip6_hdr *ip6_header;
    ip6_header = (struct ip6_hdr *)icmp6_frame->payload;

    ip6_header->ip6_flow = 0;
    ip6_header->ip6_nxt = IPPROTO_ICMPV6;
    ip6_header->ip6_plen = htons(sizeof(struct nd_neighbor_advert) + header_option_size);
    ip6_header->ip6_hlim = 0xFF;
    ip6_header->ip6_vfc = 6 << 4 | 0;

    memcpy(ip6_header->ip6_src.s6_addr, device->addr6_list[0].s6_addr, INET_ADDRSTRLEN);
    if (is_reply) {
        memcpy(ip6_header->ip6_dst.s6_addr, ipaddr->s6_addr, INET_ADDRSTRLEN);
    } else {
        inet_pton(AF_INET6, "ff02::1:ffff:ffff", &ip6_header->ip6_dst);
        ip6_header->ip6_dst.s6_addr[13] = ipaddr->s6_addr[13];
        ip6_header->ip6_dst.s6_addr[14] = ipaddr->s6_addr[14];
        ip6_header->ip6_dst.s6_addr[15] = ipaddr->s6_addr[15];
    }

    struct nd_neighbor_advert *na_header;
    na_header = (struct nd_neighbor_advert *)(ip6_header + 1);
    na_header->nd_na_type = ND_NEIGHBOR_ADVERT;
    na_header->nd_na_code = 0;
    if (is_reply) {
        na_header->nd_na_flags_reserved = ND_NA_FLAG_ROUTER | ND_NA_FLAG_SOLICITED | ND_NA_FLAG_OVERRIDE;
    } else {
        na_header->nd_na_flags_reserved = ND_NA_FLAG_ROUTER | ND_NA_FLAG_OVERRIDE;
    }
    memcpy(na_header->nd_na_target.s6_addr, target_ipaddr->s6_addr, INET_ADDRSTRLEN);

    struct nd_opt_hdr *linklayer_header = (struct nd_opt_hdr *)(na_header + 1);
    linklayer_header->nd_opt_type = ND_OPT_SOURCE_LINKADDR;
    linklayer_header->nd_opt_len = 1;
    uint8_t *linklayer_address = (uint8_t *)(linklayer_header + 1);
    memcpy(linklayer_address, device->hw_addr, sizeof(uint8_t) * ETH_ALEN);

    na_header->nd_na_cksum = calc_icmp6_checksum(ip6_header);
    return icmp6_frame;
}
