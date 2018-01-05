#include "arp_table.h"
#include "arp_waiting_list.h"
#include "ether_frame.h"
#include "log.h"
#include <net/if_arp.h>
#include <stdlib.h>
int process_arp_packet(ether_frame_t *ether_frame) {

    if (ether_frame->payload_size < sizeof(struct ether_arp)) {
        log_stdout("ARP packet is too short.\n");
        return -1;
    }

    struct ether_arp *arp_packet = (struct ether_arp *)ether_frame->payload;

    if (ntohs(arp_packet->arp_op) != ARPOP_REQUEST &&
        ntohs(arp_packet->arp_op) != ARPOP_REPLY) {
        log_stdout("ARP operation is incorrect.\n");
        return -1;
    }

    send_waiting_ethernet_frame_matching_ipaddr(*(uint32_t *)arp_packet->arp_spa,
                                                arp_packet->arp_sha);
    register_arp_table(*(uint32_t *)arp_packet->arp_spa, arp_packet->arp_sha);
    free(ether_frame->payload);
    free(ether_frame);
    return 0;
}
