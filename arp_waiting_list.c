#include "ether_frame.h"
#include "log.h"
#include "send_queue.h"
#include <net/ethernet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct arp_waiting_node {
    struct arp_waiting_node *next;
    unsigned int neighbor_ipaddr;
    ether_frame_t *ether_frame;
} arp_waiting_node_t;

static arp_waiting_node_t *head;

int init_arp_waiting_list() {
    if ((head = malloc(sizeof(arp_waiting_node_t))) == NULL) {
        log_perror("malloc");
        return -1;
    }
    head->next = NULL;
    return 0;
}

int add_arp_waiting_list(ether_frame_t *ether_frame, unsigned int neighbor_ipaddr) {
    arp_waiting_node_t *new_node;
    if ((new_node = malloc(sizeof(arp_waiting_node_t))) == NULL) {
        log_perror("malloc");
        return -1;
    }
    new_node->next = head->next;
    new_node->ether_frame = ether_frame;
    new_node->neighbor_ipaddr = neighbor_ipaddr;
    head->next = new_node;
    return 0;
}

void send_waiting_ethernet_frame_matching_ipaddr(unsigned int ipaddr, unsigned char macaddr[ETH_ALEN]) {
    arp_waiting_node_t *node = head;
    arp_waiting_node_t *sending_node = NULL;
    while (node->next != NULL) {
        if (node->next->neighbor_ipaddr == ipaddr) {
            sending_node = node->next;
            node->next = node->next->next;
            memcpy(sending_node->ether_frame->header.ether_dhost, macaddr, sizeof(unsigned char) * ETH_ALEN);
            enqueue_send_queue(sending_node->ether_frame);
            if (node->next == NULL) {
                return;
            }
        }
    }
}