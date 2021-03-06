#include "ether_frame.h"
#include "log.h"
#include "send_queue.h"
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct arp_waiting_node {
    struct arp_waiting_node *next;
    uint32_t neighbor_ipaddr;
    ether_frame_t *ether_frame;
} arp_waiting_node_t;

void delete_top_arp_node();

static arp_waiting_node_t *head;
static size_t list_size;
static const size_t LIST_SIZE_MAX = 10;

int init_arp_waiting_list() {
    if ((head = calloc(1, sizeof(arp_waiting_node_t))) == NULL) {
        log_perror("calloc");
        return -1;
    }
    head->next = NULL;
    list_size = 0;
    return 0;
}

void delete_top_arp_node() {
    arp_waiting_node_t *next = head->next->next;
    free(head->next->ether_frame);
    free(head->next);
    head->next = next;
    list_size--;
}

int add_arp_waiting_list(ether_frame_t *ether_frame, uint32_t neighbor_ipaddr) {
    if (list_size > LIST_SIZE_MAX) {
        delete_top_arp_node();
    }
    arp_waiting_node_t *new_node;
    if ((new_node = calloc(1, sizeof(arp_waiting_node_t))) == NULL) {
        log_perror("calloc");
        return -1;
    }
    new_node->next = head->next;
    new_node->ether_frame = ether_frame;
    new_node->neighbor_ipaddr = neighbor_ipaddr;
    head->next = new_node;
    list_size++;
    return 0;
}

void send_waiting_ethernet_frame_matching_ipaddr(uint32_t ipaddr, uint8_t macaddr[ETH_ALEN]) {
    arp_waiting_node_t *node = head;
    arp_waiting_node_t *sending_node = NULL;
    while (node->next != NULL) {
        if (node->next->neighbor_ipaddr == ipaddr) {
            sending_node = node->next;
            node->next = node->next->next;
            memcpy(sending_node->ether_frame->header.ether_dhost, macaddr, sizeof(uint8_t) * ETH_ALEN);
            enqueue_send_queue(sending_node->ether_frame);
            list_size--;
            if (node->next == NULL) {
                return;
            }
        }
        node = node->next;
    }
}

void print_waiting_list() {
    arp_waiting_node_t *node = head->next;
    log_stdout("---%s---\n", __func__);
    uint8_t *ptr;
    while (node != NULL) {
        ptr = (uint8_t *)&((struct iphdr *)node->ether_frame->payload)->daddr;
        log_stdout("%d.%d.%d.%d\n", ptr[0], ptr[1], ptr[2], ptr[3]);
        node = node->next;
    }
    log_stdout("---end %s---\n", __func__);
}
