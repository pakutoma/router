#include "checksum.h"
#include "ether_frame.h"
#include "log.h"
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TIME_EXCEEDED_HEADER_SIZE 8
#define TIME_EXCEEDED_DATA_SIZE 64
ether_frame_t *create_time_exceeded_request(ether_frame_t *arrived_frame, uint32_t src_ipaddr, uint32_t dst_ipaddr) {
    ether_frame_t *icmp_frame;
    struct iphdr *ip_header;
    struct icmp *icmp_header;

    if ((icmp_frame = calloc(1, sizeof(ether_frame_t))) == NULL) {
        log_perror("calloc");
        return NULL;
    }
    if ((icmp_frame->payload = calloc(1, sizeof(struct iphdr) + sizeof(uint8_t) * (TIME_EXCEEDED_DATA_SIZE + TIME_EXCEEDED_HEADER_SIZE))) == NULL) {
        log_perror("calloc");
        return NULL;
    }
    memset(icmp_frame->payload, 0xff, sizeof(struct iphdr) + sizeof(uint8_t) * (TIME_EXCEEDED_HEADER_SIZE + TIME_EXCEEDED_DATA_SIZE));
    icmp_frame->payload_size = sizeof(struct iphdr) + sizeof(uint8_t) * (TIME_EXCEEDED_HEADER_SIZE + TIME_EXCEEDED_DATA_SIZE);
    memcpy(icmp_frame->header.ether_shost, arrived_frame->header.ether_dhost, sizeof(uint8_t) * ETH_ALEN);
    memcpy(icmp_frame->header.ether_dhost, arrived_frame->header.ether_shost, sizeof(uint8_t) * ETH_ALEN);
    icmp_frame->header.ether_type = htons(ETHERTYPE_IP);
    ip_header = (struct iphdr *)icmp_frame->payload;
    ip_header->version = 4;
    ip_header->ihl = 20 / 4;
    ip_header->tos = 0;
    ip_header->tot_len = htons(sizeof(struct icmp) + TIME_EXCEEDED_DATA_SIZE); //why?
    ip_header->id = 0;
    ip_header->frag_off = 0;
    ip_header->ttl = 64;
    ip_header->protocol = IPPROTO_ICMP;
    ip_header->check = 0;
    ip_header->saddr = src_ipaddr;
    ip_header->daddr = dst_ipaddr;
    ip_header->check = calc_checksum((uint8_t *)ip_header, sizeof(struct iphdr));

    icmp_header = (struct icmp *)(ip_header + 1);
    icmp_header->icmp_type = ICMP_TIME_EXCEEDED;
    icmp_header->icmp_code = ICMP_TIMXCEED_INTRANS;
    icmp_header->icmp_cksum = 0;
    icmp_header->icmp_void = 0;
    icmp_header->icmp_cksum = 0;

    memcpy((uint8_t *)icmp_header + TIME_EXCEEDED_HEADER_SIZE, arrived_frame->payload, sizeof(uint8_t) * TIME_EXCEEDED_DATA_SIZE);

    icmp_header->icmp_cksum = calc_checksum((uint8_t *)icmp_header, TIME_EXCEEDED_HEADER_SIZE + TIME_EXCEEDED_DATA_SIZE);

    return icmp_frame;
}