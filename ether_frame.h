#pragma once
#include "device.h"
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <poll.h>
typedef struct {
    struct ether_header header;
    uint8_t *payload;
    int payload_size;
} ether_frame_t;

int receive_ethernet_frame(int epoll_fd, ether_frame_t **received_frame);
int send_ethernet_frame(device_t devices[NUMBER_OF_DEVICES], ether_frame_t *sending_frame);