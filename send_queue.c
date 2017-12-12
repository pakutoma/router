#include "ether_frame.h"
#include "log.h"
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
        log_error("SEND_QUEUE: queue empty.");
        return NULL;
    }
    ether_frame_t *data = send_queue[head];
    if (head + 1 == QUEUE_SIZE) {
        head = 0;
    } else {
        head++;
    }
    return head;
}