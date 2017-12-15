#include "ether_frame.h"
#include "device.h"
#include "log.h"
#include <errno.h>
#include <netinet/if_ether.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define ETHERNET_FRAME_SIZE 1518
#define ETHERNET_FOOTER_SIZE 4

int unpack_ethernet_frame(unsigned char buf[ETHERNET_FRAME_SIZE], int size, ether_frame_t **received_frame);

int recieve_ethernet_frame(struct pollfd sockets[2], ether_frame_t **received_frame) {
    int size;
    int status;
    int i;
    unsigned char buf[ETHERNET_FRAME_SIZE] = {0};
    status = poll(sockets, 2, 100);

    if (status == -1) { //error
        if (errno != EINTR) {
            log_perror("poll");
        }
        return -1;
    }

    if (status == 0) { //timeout
        return -1;
    }

    for (i = 0; i < 2; i++) {
        if (!(sockets[i].revents & (POLLIN | POLLERR))) {
            continue;
        }

        if ((size = read(sockets[i].fd, buf, sizeof(buf))) <= 0) {
            log_perror("read");
            return -1;
        }
    }

    if (unpack_ethernet_frame(buf, size, received_frame) == -1) {
        return -1;
    }
    return size;
}

int unpack_ethernet_frame(unsigned char buf[ETHERNET_FRAME_SIZE], int size, ether_frame_t **received_frame) {
    unsigned char *ptr = buf;

    if ((*received_frame = malloc(sizeof(ether_frame_t))) == NULL) {
        log_perror("malloc");
        return -1;
    }
    memcpy(&((*received_frame)->header), ptr, sizeof(struct ether_header));
    ptr += sizeof(struct ether_header);
    (*received_frame)->payload_size = size - sizeof(struct ether_header) - ETHERNET_FOOTER_SIZE;
    if (((*received_frame)->payload = malloc(sizeof(unsigned char) * (*received_frame)->payload_size)) == NULL) {
        log_perror("malloc");
        return -1;
    }
    memcpy((*received_frame)->payload, ptr, sizeof(unsigned char) * (*received_frame)->payload_size);
    ptr += (*received_frame)->payload_size;
    memcpy((*received_frame)->fcs, ptr, sizeof(unsigned char) * ETHERNET_FOOTER_SIZE);
    return 0;
}