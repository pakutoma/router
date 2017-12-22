#include "arp_packet.h"
#include "arp_table.h"
#include "arp_waiting_list.h"
#include "device.h"
#include "ether_frame.h"
#include "event.h"
#include "ip_packet.h"
#include "log.h"
#include "send_queue.h"
#include <arpa/inet.h>
#include <errno.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/epoll.h>

void process_packet(ether_frame_t *received_frame, device_t devices[NUMBER_OF_DEVICES], struct in_addr next_router);
void send_queued_packet(device_t devices[NUMBER_OF_DEVICES]);
bool is_sock_desc(struct epoll_event *event, device_t devices[NUMBER_OF_DEVICES]);

int route(device_t devices[NUMBER_OF_DEVICES], char *next_router_addr) {
    struct in_addr next_router;

    if (inet_aton(next_router_addr, &next_router) == 0) {
        log_error("Next router address is invalid.\n");
        return -1;
    }
    if (init_arp_table() == -1) {
        return -1;
    }
    if (init_arp_waiting_list() == -1) {
        return -1;
    }

    init_event();
    for (int i = 0; i < NUMBER_OF_DEVICES; i++) {
        add_event(devices[i].sock_desc, EPOLLIN);
    }

    struct epoll_event *event;
    while (true) {
        if ((event = wait_event()) == NULL) {
            continue;
        }

        if (is_sock_desc(event, devices)) {
            if (event->events & EPOLLIN) {
                int size;
                ether_frame_t *received_frame;
                if ((size = receive_ethernet_frame(event->data.fd, &received_frame)) > 0) {
                    process_packet(received_frame, devices, next_router);
                }
            }
            if (event->events & EPOLLOUT) {
                send_queued_packet(devices);
                modify_event(event->data.fd, EPOLLIN);
            }
        }
        /*print_waiting_list();
        print_send_queue();
        print_arp_table();*/

        ether_frame_t *ether_frame;
        if ((ether_frame = peek_send_queue()) != NULL) {
            int device_num = find_device(ether_frame->header.ether_shost, devices);
            if (device_num != -1) {
                modify_event(devices[device_num].sock_desc, EPOLLIN | EPOLLOUT);
            }
        }
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

void send_queued_packet(device_t devices[NUMBER_OF_DEVICES]) {
    ether_frame_t *sending_frame;
    sending_frame = peek_send_queue();
    if (send_ethernet_frame(devices, sending_frame) == -1) {
        return;
    } else {
        dequeue_send_queue();
        free(sending_frame->payload);
        free(sending_frame);
    }
}

bool is_sock_desc(struct epoll_event *event, device_t devices[NUMBER_OF_DEVICES]) {
    for (int i = 0; i < NUMBER_OF_DEVICES; i++) {
        if (event->data.fd == devices[i].sock_desc) {
            return true;
        }
    }
    return false;
}