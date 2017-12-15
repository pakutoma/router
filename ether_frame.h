#pragma once
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <poll.h>
typedef struct {
    struct ether_header header;
    unsigned char *payload;
    int payload_size;
    unsigned char fcs[4];

} ether_frame_t;

int receive_ethernet_frame(struct pollfd sockets[2], ether_frame_t **received_frame);