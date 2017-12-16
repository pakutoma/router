#include "ether_frame.h"
#include "checksum.h"
#include "device.h"
#include "log.h"
#include <errno.h>
#include <netinet/if_ether.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define ETHERNET_FRAME_HIGHER_LIMIT_SIZE 1518
#define ETHERNET_FRAME_LOWER_LIMIT_SIZE 64
#define ETHERNET_FOOTER_SIZE 4
#define PACKET_LOWER_LIMIT_SIZE 46

int unpack_ethernet_frame(unsigned char buf[ETHERNET_FRAME_HIGHER_LIMIT_SIZE], int size, ether_frame_t **received_frame);
int pack_ethernet_frame(unsigned char **data, ether_frame_t *sending_frame);

int receive_ethernet_frame(struct pollfd sockets[2], ether_frame_t **received_frame) {
    int status = poll(sockets, 2, 100);

    if (status == -1) { //error
        if (errno != EINTR) {
            log_perror("poll");
        }
        return -1;
    }

    if (status == 0) { //timeout
        return -1;
    }

    int size;
    unsigned char buf[ETHERNET_FRAME_HIGHER_LIMIT_SIZE] = {0};
    for (int i = 0; i < 2; i++) {
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

int unpack_ethernet_frame(unsigned char buf[ETHERNET_FRAME_HIGHER_LIMIT_SIZE], int size, ether_frame_t **received_frame) {
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
    return 0;
}

int send_ethernet_frame(device_t devices[NUMBER_OF_DEVICES], ether_frame_t *sending_frame) {
    uint8_t *data;
    int size;
    if ((size = pack_ethernet_frame(&data, sending_frame)) == -1) {
        return -1;
    }

    int index = 0;
    if ((index = find_device(sending_frame->header.ether_shost, devices)) == -1) {
        return -1;
    }

    if (write(devices[index].sock_desc, data, size) == -1) {
        log_perror("write");
        return -1;
    }

    log_stdout("---%s---\n", __func__);
    uint8_t *ptr = data;
    for (int i = 0; i < size; i++) {
        if (i % 10 == 0) {
            log_stdout("\n%d: ", i / 10 + 1);
        }
        log_stdout("%02x", *ptr);
        ptr++;
    }
    log_stdout("\n");
    log_stdout("---end %s---\n", __func__);

    free(data);
    free(sending_frame->payload);
    free(sending_frame);
    return 0;
}

int pack_ethernet_frame(unsigned char **data, ether_frame_t *sending_frame) {
    int size = sizeof(struct ether_header) + (sending_frame->payload_size * sizeof(uint8_t)) + ETHERNET_FOOTER_SIZE * sizeof(uint8_t);
    if (size > ETHERNET_FRAME_HIGHER_LIMIT_SIZE) {
        log_error("%s: Ethernet frame is too big.\n", __func__);
        return -1;
    }
    if (size < ETHERNET_FRAME_LOWER_LIMIT_SIZE) {
        size = ETHERNET_FRAME_LOWER_LIMIT_SIZE;
    }

    if ((*data = calloc(size, sizeof(uint8_t))) == NULL) {
        log_perror("calloc");
        return -1;
    }

    uint8_t *ptr = *data;
    memcpy(ptr, &sending_frame->header, sizeof(struct ether_header));
    ptr += sizeof(struct ether_header);
    memcpy(ptr, sending_frame->payload, sending_frame->payload_size * sizeof(uint8_t));
    ptr += sending_frame->payload_size;
    if (sending_frame->payload_size < PACKET_LOWER_LIMIT_SIZE) {
        int padding_size = PACKET_LOWER_LIMIT_SIZE - sending_frame->payload_size;
        ptr += padding_size;
    }
    uint32_t fcs = htonl(calc_fcs(ptr, size - ETHERNET_FOOTER_SIZE));
    memcpy(ptr, &fcs, ETHERNET_FOOTER_SIZE * sizeof(uint8_t));
    return size;
}
