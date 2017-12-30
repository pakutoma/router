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

static struct msghdr *(*send_queue)[QUEUE_SIZE];
static int *head;
static int *tail;

struct msghdr *pack_msghdr(void *packed_frame, size_t size);
struct msghdr *dequeue_send_queue(int index);

int init_send_queue() {
    if ((send_queue = calloc(get_devices_length() * QUEUE_SIZE, sizeof(struct mmsghdr *))) == NULL) {
        log_perror("calloc");
        return -1;
    }
    if ((head = calloc(get_devices_length(), sizeof(int *))) == NULL) {
        log_perror("calloc");
        free(send_queue);
        return -1;
    }
    if ((tail = calloc(get_devices_length(), sizeof(int *))) == NULL) {
        log_perror("calloc");
        free(send_queue);
        free(head);
        return -1;
    }
    return 0;
}

int enqueue_send_queue(ether_frame_t *ether_frame) {

    int index = find_device_index_by_macaddr(ether_frame->header.ether_shost);
    uint8_t *packed_frame;
    size_t size = pack_ethernet_frame(&packed_frame, ether_frame);
    struct msghdr *msg_hdr = pack_msghdr(packed_frame, size);

    free(ether_frame->payload);
    free(ether_frame);

    if (head[index] == (tail[index] + 1) % QUEUE_SIZE) {
        log_error("SEND_QUEUE: queue overflow.\n");
        return -1;
    }
    send_queue[index][tail[index]] = msg_hdr;
    if (tail[index] + 1 == QUEUE_SIZE) {
        tail[index] = 0;
    } else {
        tail[index]++;
    }
    return 0;
}

struct msghdr *dequeue_send_queue(int index) {
    if (head[index] == tail[index]) {
        return NULL;
    }
    struct msghdr *data = send_queue[index][head[index]];
    if (head[index] + 1 == QUEUE_SIZE) {
        head[index] = 0;
    } else {
        head[index]++;
    }
    return data;
}

struct mmsghdr *get_mmsghdrs(int index, int length) {
    struct mmsghdr *mmsg_hdrs = calloc(length, sizeof(struct mmsghdr));

    for (int i = 0; i < length; i++) {
        struct msghdr *msg_hdr = dequeue_send_queue(index);
        if (msg_hdr == NULL) {
            free(mmsg_hdrs);
            return NULL;
        }
        memcpy(&mmsg_hdrs[i].msg_hdr, msg_hdr, sizeof(struct msghdr));
        free(msg_hdr);
    }
    return mmsg_hdrs;
}

int get_size_of_send_queue(int index) {
    if (head[index] <= tail[index]) {
        return tail[index] - head[index];
    } else {
        return QUEUE_SIZE - head[index] + tail[index];
    }
}

struct msghdr *pack_msghdr(void *data, size_t size) {
    struct msghdr *msg_hdr = calloc(1, sizeof(struct msghdr));
    struct iovec *iov = calloc(1, sizeof(struct iovec));
    iov->iov_base = data;
    iov->iov_len = size;
    msg_hdr->msg_iov = iov;
    msg_hdr->msg_iovlen = 1;
    msg_hdr->msg_name = NULL;
    msg_hdr->msg_namelen = 0;
    msg_hdr->msg_control = NULL;
    msg_hdr->msg_controllen = 0;
    return msg_hdr;
}
