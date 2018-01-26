#include "log.h"
#include <net/ethernet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define ARP_TABLE_TIMEOUT 120
typedef struct ip_mac_node {
    struct ip_mac_node *next;
    uint32_t ipaddr;
    uint8_t macaddr[ETH_ALEN];
    time_t creation_time;
} ip_mac_node_t;

void remove_timeout_cache();

static ip_mac_node_t *head = NULL;

int init_arp_table() {
    if ((head = calloc(1, sizeof(ip_mac_node_t))) == NULL) {
        log_perror("calloc");
        return -1;
    }
    head->next = NULL;
    return 0;
}

int register_arp_table(uint32_t new_ipaddr, uint8_t new_macaddr[ETH_ALEN]) {
    ip_mac_node_t *node = head;
    while (node != NULL) {
        if (node->ipaddr == new_ipaddr) {
            if (memcmp(node->macaddr, new_macaddr, ETH_ALEN * sizeof(uint8_t))) {
                node->creation_time = time(NULL);
            } else {
                memcpy(node->macaddr, new_macaddr, ETH_ALEN * sizeof(uint8_t));
                node->creation_time = time(NULL);
            }
            return 0;
        }
        node = node->next;
    }

    ip_mac_node_t *new_node;
    if ((new_node = calloc(1, sizeof(ip_mac_node_t))) == NULL) {
        log_perror("calloc");
        return -1;
    }
    new_node->next = head->next;
    new_node->ipaddr = new_ipaddr;
    new_node->creation_time = time(NULL);
    memcpy(new_node->macaddr, new_macaddr, ETH_ALEN * sizeof(uint8_t));
    head->next = new_node;
    return 0;
}

int find_macaddr(uint32_t ipaddr, uint8_t macaddr[ETH_ALEN]) {
    ip_mac_node_t *node = head->next;
    while (node != NULL) {
        if (node->ipaddr == ipaddr) {
            memcpy(macaddr, node->macaddr, sizeof(uint8_t) * ETH_ALEN);
            return 0;
        }
        node = node->next;
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
        node = node->next;
    }
}

void print_arp_table() {
    ip_mac_node_t *node = head->next;
    log_stdout("---%s---\n", __func__);
    uint8_t *ptr;
    time_t now = time(NULL);
    while (node != NULL) {
        ptr = (uint8_t *)&node->ipaddr;
        log_stdout("%d.%d.%d.%d->", ptr[0], ptr[1], ptr[2], ptr[3]);
        ptr = node->macaddr;
        log_stdout("%02x:%02x:%02x:%02x:%02x:%02x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
        log_stdout("(%d sec)\n", now - node->creation_time);
        node = node->next;
    }
    log_stdout("---end %s---\n", __func__);
}
