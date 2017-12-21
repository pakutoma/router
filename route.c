#include "arp_packet.h"
#include "arp_table.h"
#include "arp_waiting_list.h"
#include "device.h"
#include "ether_frame.h"
#include "ip_packet.h"
#include "log.h"
#include "send_queue.h"
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/epoll.h>

void process_packet(ether_frame_t *received_frame, device_t devices[NUMBER_OF_DEVICES], struct in_addr next_router);
void send_queued_packets(device_t devices[NUMBER_OF_DEVICES]);

int route(device_t devices[2], char *next_router_addr) {
    struct in_addr next_router;

    if (inet_aton(next_router_addr, &next_router) == 0) {
        log_error("Next router address is invalid.\n");
        return -1;
    }

    int epoll_fd;
    if ((epoll_fd = epoll_create1(0)) == -1) {
        log_perror("epoll_create1");
        return -1;
    }

    struct epoll_event ev;
    for (int i = 0; i < NUMBER_OF_DEVICES; i++) {
        ev.data.fd = devices[i].sock_desc;
        ev.events = POLLIN;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, devices[i].sock_desc, &ev) == -1) {
            log_perror("epoll_ctl");
            return -1;
        }
    }

    if (init_arp_table() == -1) {
        return -1;
    }
    if (init_arp_waiting_list() == -1) {
        return -1;
    }

    while (1) {
        int size;
        ether_frame_t *received_frame;
        if ((size = receive_ethernet_frame(epoll_fd, &received_frame)) > 0) {
            process_packet(received_frame, devices, next_router);
        }
        print_waiting_list();
        print_send_queue();
        print_arp_table();
        send_queued_packets(devices);
    }
    return 0;
}

void process_packet(ether_frame_t *received_frame, device_t devices[NUMBER_OF_DEVICES], struct in_addr next_router) {
    switch (ntohs(received_frame->header.ether_type)) {
        case ETHERTYPE_IP:
            log_stdout("ETHERTYPE: IP\n");
            if (process_ip_packet(received_frame, devices, next_router) == -1) {
                free(received_frame->payload);
                free(received_frame);
            }
            break;
        case ETHERTYPE_ARP:
            log_stdout("ETHERTYPE: ARP\n");
            if (process_arp_packet(received_frame) == -1) {
                free(received_frame->payload);
                free(received_frame);
            }
            break;
        case ETHERTYPE_IPV6:
            log_stdout("ETHERTYPE: IPv6\n");
            //process_ipv6_packet(received_frame);
            free(received_frame->payload);
            free(received_frame);
            break;
        default:
            log_stdout("ETHERTYPE: UNKNOWN\n");
            free(received_frame->payload);
            free(received_frame);
            break;
    }
}

void send_queued_packets(device_t devices[NUMBER_OF_DEVICES]) {
    ether_frame_t *sending_frame;
    while ((sending_frame = peek_send_queue()) != NULL) {
        if (send_ethernet_frame(devices, sending_frame) == -1) {
            break;
        } else {
            sending_frame = dequeue_send_queue();
            free(sending_frame->payload);
            free(sending_frame);
        }
    }
}