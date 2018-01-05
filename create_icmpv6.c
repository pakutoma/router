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

    int header_option_size = sizeof(struct nd_opt_hdr) + sizeof(uint8_t) * 6;
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

    ip6_header->ip6_src = device->addr6_list[0];
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
    log_error("checksum: %x\n", ra_header->nd_ra_cksum);

    uint8_t *buf;
    int size = pack_ethernet_frame(&buf, icmp6_frame);
    log_stdout("---%s---", __func__);
    uint8_t *ptr = buf;
    for (int i = 0; i < size; i++) {
        if (i % 10 == 0) {
            log_stdout("\n%04d: ", i / 10 + 1);
        }
        log_stdout("%02x ", *ptr);
        ptr++;
    }
    log_stdout("\n");
    log_stdout("---end %s---\n", __func__);

    return icmp6_frame;
}
