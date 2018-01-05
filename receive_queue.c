#define _GNU_SOURCE
#include "ether_frame.h"
#include "log.h"
#include "settings.h"
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#define QUEUE_SIZE 2000

static struct mmsghdr *(*receive_queue)[QUEUE_SIZE];
static int *head;
static int *tail;

size_t unpack_mmsghdr(void **data, struct mmsghdr *mmsg_hdr);

int init_receive_queue() {
    if ((receive_queue = calloc(get_devices_length() * QUEUE_SIZE, sizeof(struct mmsghdr *))) == NULL) {
        log_perror("calloc");
        return -1;
    }
    if ((head = calloc(get_devices_length(), sizeof(int))) == NULL) {
        log_perror("calloc");
        free(receive_queue);
        return -1;
    }
    if ((tail = calloc(get_devices_length(), sizeof(int))) == NULL) {
        log_perror("calloc");
        free(receive_queue);
        free(head);
        return -1;
    }
    return 0;
}

int enqueue_receive_queue(int index, struct mmsghdr *mmsg_hdr) {
    if (head[index] == (tail[index] + 1) % QUEUE_SIZE) {
        log_error("receive_queue: queue overflow.\n");
        return -1;
    }
    receive_queue[index][tail[index]] = mmsg_hdr;
    if (tail[index] + 1 == QUEUE_SIZE) {
        tail[index] = 0;
    } else {
        tail[index]++;
    }
    return 0;
}

ether_frame_t *dequeue_receive_queue() {
    int index = -1;
    for (size_t i = 0; i < get_devices_length(); i++) {
        if (head[i] != tail[i]) {
            index = i;
        }
    }
    if (index == -1) {
        return NULL;
    }
    struct mmsghdr *mmsg_hdr = receive_queue[index][head[index]];
    void *raw_frame;
    size_t size = unpack_mmsghdr(&raw_frame, mmsg_hdr);
    ether_frame_t *unpacked_frame;
    unpack_ethernet_frame(raw_frame, size, &unpacked_frame, index);

    if (head[index] + 1 == QUEUE_SIZE) {
        head[index] = 0;
    } else {
        head[index]++;
    }

    return unpacked_frame;
}

int set_mmsghdrs(int index, struct mmsghdr *mmsg_hdrs, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (enqueue_receive_queue(index, &mmsg_hdrs[i]) == -1) {
            return -1;
        }
    }
    return 0;
}

int get_size_of_receive_queue(int index) {
    if (head[index] <= tail[index]) {
        return tail[index] - head[index];
    } else {
        return QUEUE_SIZE - head[index] + tail[index];
    }
}

size_t unpack_mmsghdr(void **data, struct mmsghdr *mmsg_hdr) {
    struct iovec *iov = mmsg_hdr->msg_hdr.msg_iov;
    *data = iov->iov_base;
    return mmsg_hdr->msg_len;
}
