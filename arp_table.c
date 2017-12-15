#include "log.h"
#include <net/ethernet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define ARP_TABLE_TIMEOUT 120
typedef struct ip_mac_node {
    struct ip_mac_node *next;
    unsigned int ipaddr;
    unsigned char macaddr[ETH_ALEN];
    time_t creation_time;
} ip_mac_node_t;

void remove_timeout_cache();

static ip_mac_node_t *head = NULL;

int init_arp_table() {
    if ((head = malloc(sizeof(ip_mac_node_t))) == NULL) {
        log_perror("malloc");
        return -1;
    }
    head->next = NULL;
    return 0;
}

int register_arp_table(unsigned int ipaddr, unsigned char macaddr[ETH_ALEN]) {
    ip_mac_node_t *new_node;
    if ((new_node = malloc(sizeof(ip_mac_node_t))) == NULL) {
        log_perror("malloc");
        return -1;
    }
    new_node->next = head->next;
    new_node->ipaddr = ipaddr;
    memcpy(new_node->macaddr, macaddr, sizeof(unsigned char) * ETH_ALEN);
    head->next = new_node;
    return 0;
}

int find_macaddr(unsigned int ipaddr, unsigned char macaddr[ETH_ALEN]) {
    remove_timeout_cache();
    ip_mac_node_t *node = head->next;
    while (node != NULL) {
        if (node->ipaddr == ipaddr) {
            memcpy(macaddr, node->macaddr, sizeof(unsigned char) * ETH_ALEN);
            return 0;
        }
    }
    return -1;
}

void remove_timeout_cache() {
    ip_mac_node_t *node = head;
    ip_mac_node_t *obsolete_node = NULL;
    time_t now = time(NULL);
    while (node->next != NULL) {
        if ((now - node->next->creation_time) > ARP_TABLE_TIMEOUT) {
            obsolete_node = node->next;
            node->next = node->next->next;
            free(obsolete_node);
            if (node->next == NULL) {
                return;
            }
        }
    }
}