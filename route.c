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
#include <poll.h>
#include <signal.h>
#include <stdlib.h>

static int is_end = 0;

void end_signal(int signal);

int route(device_t devices[2], char *next_router_addr) {
    ether_frame_t *received_frame;
    int size;
    struct in_addr next_router;

    if (inet_aton(next_router_addr, &next_router) == 0) {
        log_error("Next router address is invalid.\n");
        return -1;
    }

    struct pollfd sockets[2];
    sockets[0].fd = devices[0].sock_desc;
    sockets[0].events = POLLIN;
    sockets[1].fd = devices[1].sock_desc;
    sockets[1].events = POLLIN;

    signal(SIGINT, end_signal);
    signal(SIGTERM, end_signal);
    signal(SIGQUIT, end_signal);

    if (init_arp_table() == -1) {
        return -1;
    }
    if (init_arp_waiting_list() == -1) {
        return -1;
    }

    while (!is_end) {
        if ((size = receive_ethernet_frame(sockets, &received_frame)) <= 0) {
            continue;
        }
        print_waiting_list();
        print_send_queue();
        switch (ntohs(received_frame->header.ether_type)) {
            case ETHERTYPE_IP:
                log_stdout("ETHERTYPE: IP\n");
                process_ip_packet(received_frame, devices, next_router);
                break;
            case ETHERTYPE_ARP:
                log_stdout("ETHERTYPE: ARP\n");
                //process_arp_packet(received_frame);
                break;
            case ETHERTYPE_IPV6:
                log_stdout("ETHERTYPE: IPv6\n");
                //process_ipv6_packet(received_frame);
                break;
            default:
                log_stdout("ETHERTYPE: UNKNOWN\n");
                break;
        }

        ether_frame_t *sending_frame;
        while ((sending_frame = dequeue_send_queue()) != NULL) {
            send_ethernet_frame(devices, sending_frame);
        }
    }
    return 0;
}

void end_signal(int signal) {
    is_end = 1;
}