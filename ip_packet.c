#include "arp_table.h"
#include "arp_waiting_list.h"
#include "checksum.h"
#include "create_arp.h"
#include "create_icmp.h"
#include "device.h"
#include "ether_frame.h"
#include "log.h"
#include "send_queue.h"
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <stdint.h>
#include <string.h>
int decrement_ttl(struct iphdr *header);
int validate_ip_packet(ether_frame_t *ether_frame, struct iphdr *header);
int find_recieved_device(uint8_t macaddr[ETH_ALEN], device_t devices[NUMBER_OF_DEVICES]);
int find_device_neighbor_network(uint32_t ipaddr, device_t devices[NUMBER_OF_DEVICES]);
int send_packet(ether_frame_t *ether_frame, uint8_t src_macaddr[ETH_ALEN], uint32_t origin_device_ipaddr, uint32_t neighbor_ipaddr);
int process_ip_packet(ether_frame_t *ether_frame, device_t devices[NUMBER_OF_DEVICES], struct in_addr next_router) {
    if (ether_frame->payload_size < sizeof(struct iphdr)) {
        log_stdout("IP packet is too short.\n");
        return -1;
    }

    struct iphdr *ip_header = (struct iphdr *)ether_frame->payload;
    if (validate_ip_packet(ether_frame, ip_header) == -1) {
        log_stdout("IP packet validate failed.\n");
        return -1;
    }

    int recieved_device_index;
    if ((recieved_device_index = find_device(ether_frame->header.ether_dhost, devices)) == -1) {
        log_stdout("Couldn't find recieved device.\n");
        return -1;
    }

    if (decrement_ttl(ip_header) == 0) {
        log_stdout("IP packet is time exceeded.\n");
        ether_frame_t *icmp_frame;
        if ((icmp_frame = create_time_exceeded_request(ether_frame, devices[recieved_device_index].addr.s_addr, ip_header->saddr)) == NULL) {
            return -1;
        }
        enqueue_send_queue(icmp_frame);
        return -1;
    }

    int out_device_index;
    if ((out_device_index = find_device_neighbor_network(ip_header->daddr, devices)) == -1) {
        if (send_packet(ether_frame, devices[GATEWAY_DEVICE].hw_addr, devices[GATEWAY_DEVICE].addr.s_addr, (uint32_t)next_router.s_addr) == -1) {
            return -1;
        }
    } else {
        if (send_packet(ether_frame, devices[out_device_index].hw_addr, devices[out_device_index].addr.s_addr, ip_header->daddr)) {
            return -1;
        }
    }
    return 0;
}

int send_packet(ether_frame_t *ether_frame, uint8_t src_macaddr[ETH_ALEN], uint32_t origin_device_ipaddr, uint32_t neighbor_ipaddr) {
    uint8_t dst_macaddr[ETH_ALEN];

    if (find_macaddr(neighbor_ipaddr, dst_macaddr) == -1) {
        ether_frame_t *arp_frame;
        if ((arp_frame = create_arp_request(ether_frame, origin_device_ipaddr, neighbor_ipaddr)) == NULL) {
            return -1;
        }
        enqueue_send_queue(arp_frame);
        memcpy(ether_frame->header.ether_shost, src_macaddr, sizeof(uint8_t) * ETH_ALEN);
        add_arp_waiting_list(ether_frame, neighbor_ipaddr);
    } else {
        memcpy(ether_frame->header.ether_dhost, dst_macaddr, sizeof(uint8_t) * ETH_ALEN);
        memcpy(ether_frame->header.ether_shost, src_macaddr, sizeof(uint8_t) * ETH_ALEN);
        enqueue_send_queue(ether_frame);
    }
    return 0;
}

int validate_ip_packet(ether_frame_t *ether_frame, struct iphdr *header) {
    int header_length = header->ihl * 4;
    uint16_t checksum = header->check;
    header->check = 0;
    if (checksum != calc_checksum((uint8_t *)header, header_length)) {
        return -1;
    }
    return 0;
}

int decrement_ttl(struct iphdr *header) {
    int ttl = header->ttl - 1;
    if (ttl == 0) {
        return 0;
    }
    header->check = update_checksum(header->check, header->ttl, ttl);
    return ttl;
}

int find_device_neighbor_network(uint32_t ipaddr, device_t devices[NUMBER_OF_DEVICES]) {
    int i;
    for (i = 0; i < NUMBER_OF_DEVICES; i++) {
        if (devices[i].subnet.s_addr == (ipaddr & devices[i].netmask.s_addr)) {
            return i;
        }
    }
    return -1;
}