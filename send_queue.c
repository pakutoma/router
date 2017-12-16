#include "ether_frame.h"
#include "log.h"
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <netinet/ip.h>
#include <stdio.h>
#define QUEUE_SIZE 100

static ether_frame_t *send_queue[QUEUE_SIZE];
static int head = 0;
static int tail = 0;

int enqueue_send_queue(ether_frame_t *ether_frame) {
    if (head == (tail + 1) % QUEUE_SIZE) {
        log_error("SEND_QUEUE: queue overflow.\n");
        return -1;
    }
    send_queue[tail] = ether_frame;
    if (tail + 1 == QUEUE_SIZE) {
        tail = 0;
    } else {
        tail++;
    }
    return 0;
}

ether_frame_t *dequeue_send_queue() {
    if (head == tail) {
        return NULL;
    }
    ether_frame_t *data = send_queue[head];
    uint8_t *ptr;

    log_stdout("---%s---\n", __func__);
    if (ntohs(data->header.ether_type) == ETHERTYPE_IP) {
        log_stdout("ETHERTYPE: IP\n");
        ptr = (uint8_t *)&((struct iphdr *)data->payload)->daddr;
        log_stdout("dst: %d.%d.%d.%d\n", ptr[0], ptr[1], ptr[2], ptr[3]);
        ptr = (uint8_t *)&((struct iphdr *)data->payload)->saddr;
        log_stdout("src: %d.%d.%d.%d\n", ptr[0], ptr[1], ptr[2], ptr[3]);
    } else if (ntohs(data->header.ether_type) == ETHERTYPE_ARP) {
        log_stdout("ETHERTYPE: ARP\n");
        ptr = (uint8_t *)&((struct ether_arp *)data->payload)->arp_tpa;
        log_stdout("dst: %d.%d.%d.%d\n", ptr[0], ptr[1], ptr[2], ptr[3]);
        ptr = (uint8_t *)&((struct ether_arp *)data->payload)->arp_spa;
        log_stdout("src: %d.%d.%d.%d\n", ptr[0], ptr[1], ptr[2], ptr[3]);
    }
    log_stdout("---end %s---\n", __func__);

    if (head + 1 == QUEUE_SIZE) {
        head = 0;
    } else {
        head++;
    }
    return data;
}

void print_send_queue() {
    log_stdout("---%s---\n", __func__);
    uint8_t *ptr;
    int index = head;
    while (index != tail) {
        if (ntohs(send_queue[index]->header.ether_type) == ETHERTYPE_IP) {
            log_stdout("ETHERTYPE: IP\n");
            ptr = (uint8_t *)&((struct iphdr *)send_queue[index]->payload)->daddr;
            log_stdout("dst: %d.%d.%d.%d\n", ptr[0], ptr[1], ptr[2], ptr[3]);
            ptr = (uint8_t *)&((struct iphdr *)send_queue[index]->payload)->saddr;
            log_stdout("src: %d.%d.%d.%d\n", ptr[0], ptr[1], ptr[2], ptr[3]);
        } else if (ntohs(send_queue[index]->header.ether_type) == ETHERTYPE_ARP) {
            log_stdout("ETHERTYPE: ARP\n");
            ptr = (uint8_t *)&((struct ether_arp *)send_queue[index]->payload)->arp_tpa;
            log_stdout("dst: %d.%d.%d.%d\n", ptr[0], ptr[1], ptr[2], ptr[3]);
            ptr = (uint8_t *)&((struct ether_arp *)send_queue[index]->payload)->arp_spa;
            log_stdout("src: %d.%d.%d.%d\n", ptr[0], ptr[1], ptr[2], ptr[3]);
        }
        if (index + 1 == QUEUE_SIZE) {
            index = 0;
        } else {
            index++;
        }
    }
    log_stdout("---end %s---\n", __func__);
}