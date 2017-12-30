#define _GNU_SOURCE
#include "arp_packet.h"
#include "arp_table.h"
#include "arp_waiting_list.h"
#include "ether_frame.h"
#include "event.h"
#include "ip_packet.h"
#include "log.h"
#include "receive_queue.h"
#include "send_queue.h"
#include "settings.h"
#include "timer.h"
#include <arpa/inet.h>
#include <errno.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/uio.h>
#include <unistd.h>

void process_packet(ether_frame_t *received_frame);
void send_frames(int sock_desc);
void receive_frames(int sock_desc, struct mmsghdr *mmsg_hdrs);
bool is_sock_desc(struct epoll_event *event);

static int sec_timer_fd;
static const struct itimerspec sec_timer = {{1, 0}, {1, 0}};
static struct mmsghdr mmsg_hdrs[UIO_MAXIOV];

int route() {
    struct epoll_event *event;

    while (true) {
        if ((event = wait_event()) == NULL) {
            continue;
        }

        if (is_sock_desc(event)) {
            if (event->events & EPOLLIN) {
                receive_frames(event->data.fd, mmsg_hdrs);
                ether_frame_t *received_frame;
                while ((received_frame = dequeue_receive_queue()) != NULL) {
                    process_packet(received_frame);
                }
            }
            if (event->events & EPOLLOUT) {
                send_frames(event->data.fd);
                modify_event(event->data.fd, EPOLLIN);
            }
        }
        if (event->data.fd == sec_timer_fd) {
            uint64_t t;
            read(sec_timer_fd, &t, sizeof(uint64_t));
            remove_timeout_cache();
        }
        /*print_waiting_list();
        print_arp_table();*/

        for (int i = 0; i < get_devices_length(); i++) {
            if (get_size_of_send_queue(i) > 0) {
                device_t *device = get_device(i);
                modify_event(device->sock_desc, EPOLLIN | EPOLLOUT);
            }
        }
    }

    //上のループは抜けないが一応free
    for (size_t i = 0; i < UIO_MAXIOV; i++) {
        free(mmsg_hdrs[i].msg_hdr.msg_iov->iov_base);
        free(mmsg_hdrs[i].msg_hdr.msg_iov);
        free(mmsg_hdrs[i].msg_hdr.msg_control);
    }
    return 0;
}

int init_route() {
    if (init_arp_table() == -1) {
        return -1;
    }
    if (init_arp_waiting_list() == -1) {
        return -1;
    }
    if (init_event() == -1) {
        return -1;
    }
    if (init_send_queue() == -1) {
        return -1;
    }
    if (init_receive_queue() == -1) {
        return -1;
    }

    for (int i = 0; i < get_devices_length(); i++) {
        if (add_event(get_device(i)->sock_desc, EPOLLIN) == -1) {
            return -1;
        }
    }

    sec_timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (sec_timer_fd == -1) {
        log_perror("timerfd_create");
        return -1;
    }
    if (timerfd_settime(sec_timer_fd, 0, &sec_timer, NULL) == -1) {
        return -1;
    }
    if (add_event(sec_timer_fd, EPOLLIN) == -1) {
        return -1;
    }

    for (size_t i = 0; i < UIO_MAXIOV; i++) {
        mmsg_hdrs[i].msg_hdr.msg_iov = calloc(1, sizeof(struct iovec));
        mmsg_hdrs[i].msg_hdr.msg_iov->iov_base = calloc(ETHERNET_FRAME_HIGHER_LIMIT_SIZE, sizeof(uint8_t));
        mmsg_hdrs[i].msg_hdr.msg_iov->iov_len = ETHERNET_FRAME_HIGHER_LIMIT_SIZE;
        mmsg_hdrs[i].msg_hdr.msg_iovlen = 1;
        mmsg_hdrs[i].msg_hdr.msg_name = NULL;
        mmsg_hdrs[i].msg_hdr.msg_namelen = 0;
        mmsg_hdrs[i].msg_hdr.msg_control = calloc(ETHERNET_FRAME_HIGHER_LIMIT_SIZE, sizeof(uint8_t));
        mmsg_hdrs[i].msg_hdr.msg_controllen = ETHERNET_FRAME_HIGHER_LIMIT_SIZE;
        mmsg_hdrs[i].msg_hdr.msg_flags = 0;
    }
    return 0;
}

void process_packet(ether_frame_t *received_frame) {
    switch (ntohs(received_frame->header.ether_type)) {
        case ETHERTYPE_IP:
            log_stdout("ETHERTYPE: IP\n");
            if (process_ip_packet(received_frame) == -1) {
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

void send_frames(int sock_desc) {
    int index = find_device_index_by_sock_desc(sock_desc);
    int length = get_size_of_send_queue(index);
    struct mmsghdr *mmsg_hdrs = get_mmsghdrs(index, length);
    if (sendmmsg(sock_desc, mmsg_hdrs, length, 0) == -1) {
        log_perror("sendmmsg");
        return;
    }
    for (int i = 0; i < length; i++) {
        free(mmsg_hdrs[i].msg_hdr.msg_iov->iov_base);
        free(mmsg_hdrs[i].msg_hdr.msg_iov);
    }
    free(mmsg_hdrs);
}

void receive_frames(int sock_desc, struct mmsghdr *mmsg_hdrs) {
    int index = find_device_index_by_sock_desc(sock_desc);
    size_t length;
    if ((length = recvmmsg(sock_desc, mmsg_hdrs, UIO_MAXIOV, MSG_DONTWAIT, NULL)) == -1) {
        log_perror("recvmmsg");
        return;
    }
    set_mmsghdrs(index, mmsg_hdrs, length);
}

bool is_sock_desc(struct epoll_event *event) {
    for (int i = 0; i < get_devices_length(); i++) {
        if (event->data.fd == get_device(i)->sock_desc) {
            return true;
        }
    }
    return false;
}
