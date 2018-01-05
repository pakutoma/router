#pragma once
#include "settings.h"
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <poll.h>
typedef struct {
    struct ether_header header;
    uint8_t *payload;
    int payload_size;
    bool has_index;
    int index;
} ether_frame_t;

#define ETHERNET_FRAME_HIGHER_LIMIT_SIZE 1514

int receive_ethernet_frame(int epoll_fd, ether_frame_t **received_frame);
size_t pack_ethernet_frame(uint8_t **data, ether_frame_t *sending_frame);
int unpack_ethernet_frame(uint8_t buf[ETHERNET_FRAME_HIGHER_LIMIT_SIZE], int size, ether_frame_t **received_frame,int index);
