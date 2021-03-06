#include "ether_frame.h"
#include "log.h"
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
ether_frame_t *create_arp_request(uint8_t own_macaddr[ETH_ALEN], uint32_t own_ipaddr, uint32_t neighbor_ipaddr) {
    ether_frame_t *arp_frame;
    struct ether_arp *arp_packet;
    if ((arp_frame = calloc(1, sizeof(ether_frame_t))) == NULL) {
        log_perror("calloc");
        return NULL;
    }
    if ((arp_frame->payload = calloc(1, sizeof(struct ether_arp))) == NULL) {
        log_perror("calloc");
        return NULL;
    }
    arp_frame->payload_size = sizeof(struct ether_arp);
    memset(arp_frame->header.ether_dhost, 0xff, sizeof(uint8_t) * ETH_ALEN);
    memcpy(arp_frame->header.ether_shost, own_macaddr, sizeof(uint8_t) * ETH_ALEN);
    arp_frame->header.ether_type = htons(ETHERTYPE_ARP);
    arp_packet = (struct ether_arp *)arp_frame->payload;
    arp_packet->arp_hrd = htons(ARPHRD_ETHER);
    arp_packet->arp_pro = htons(ETHERTYPE_IP);
    arp_packet->arp_hln = 6;
    arp_packet->arp_pln = 4;
    arp_packet->arp_op = htons(ARPOP_REQUEST);
    memcpy(arp_packet->arp_sha, own_macaddr, sizeof(uint8_t) * ETH_ALEN);
    memset(arp_packet->arp_tha, 0, sizeof(uint8_t) * ETH_ALEN);
    memcpy(arp_packet->arp_spa, (uint8_t *)&own_ipaddr, sizeof(uint32_t));
    memcpy(arp_packet->arp_tpa, (uint8_t *)&neighbor_ipaddr, sizeof(uint32_t));
    return arp_frame;
}