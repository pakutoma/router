#include "ether_frame.h"
#include "log.h"
#include "send_queue.h"
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct ndp_waiting_node {
    struct ndp_waiting_node *next;
    struct in6_addr neighbor_ipaddr;
    ether_frame_t *ether_frame;
} ndp_waiting_node_t;

void delete_top_ndp_node();

static ndp_waiting_node_t *head;
static size_t list_size;
static const size_t LIST_SIZE_MAX = 10;

int init_ndp_waiting_list() {
    if ((head = calloc(1, sizeof(ndp_waiting_node_t))) == NULL) {
        log_perror("calloc");
        return -1;
    }
    head->next = NULL;
    list_size = 0;
    return 0;
}

void delete_top_ndp_node() {
    ndp_waiting_node_t *next = head->next->next;
    free(head->next->ether_frame);
    free(head->next);
    head->next = next;
    list_size--;
}

int add_ndp_waiting_list(ether_frame_t *ether_frame, struct in6_addr *neighbor_ipaddr) {
    if (list_size > LIST_SIZE_MAX) {
        delete_top_ndp_node();
    }
    ndp_waiting_node_t *new_node;
    if ((new_node = calloc(1, sizeof(ndp_waiting_node_t))) == NULL) {
        log_perror("calloc");
        return -1;
    }
    new_node->next = head->next;
    new_node->ether_frame = ether_frame;
    new_node->neighbor_ipaddr = *neighbor_ipaddr;
    head->next = new_node;
    list_size++;
    return 0;
}

void send_waiting_frame(struct in6_addr *ipaddr, struct ether_addr *macaddr) {
    ndp_waiting_node_t *node = head;
    ndp_waiting_node_t *sending_node = NULL;
    while (node->next != NULL) {
        if (memcmp(node->next->neighbor_ipaddr.s6_addr, ipaddr->s6_addr, sizeof(struct in6_addr)) == 0) {
            sending_node = node->next;
            node->next = node->next->next;
            memcpy(sending_node->ether_frame->header.ether_dhost, macaddr->ether_addr_octet, sizeof(uint8_t) * ETH_ALEN);
            enqueue_send_queue(sending_node->ether_frame);
            list_size--;
            if (node->next == NULL) {
                return;
            }
        }
        node = node->next;
    }
}
