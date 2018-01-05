#include "checksum.h"
#include "ether_frame.h"
#include "log.h"
#include "settings.h"
#include "timer.h"
#include <errno.h>
#include <netinet/if_ether.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#define ETHERNET_FRAME_LOWER_LIMIT_SIZE 64
#define PACKET_LOWER_LIMIT_SIZE 46

int unpack_ethernet_frame(uint8_t buf[ETHERNET_FRAME_HIGHER_LIMIT_SIZE], int size, ether_frame_t **received_frame, int index) {
    uint8_t *ptr = buf;

    if ((*received_frame = calloc(1, sizeof(ether_frame_t))) == NULL) {
        log_perror("calloc");
        return -1;
    }
    memcpy(&((*received_frame)->header), ptr, sizeof(struct ether_header));
    ptr += sizeof(struct ether_header);
    (*received_frame)->payload_size = size - sizeof(struct ether_header);
    if (((*received_frame)->payload = calloc((*received_frame)->payload_size, sizeof(uint8_t))) == NULL) {
        log_perror("calloc");
        return -1;
    }
    memcpy((*received_frame)->payload, ptr, sizeof(uint8_t) * (*received_frame)->payload_size);
    ptr += (*received_frame)->payload_size;

    (*received_frame)->has_index = true;
    (*received_frame)->index = index;

    return 0;
}

size_t pack_ethernet_frame(uint8_t **data, ether_frame_t *sending_frame) {
    int size = sizeof(struct ether_header) + (sending_frame->payload_size * sizeof(uint8_t));
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
#if DEBUG
        log_stdout("add padding.\n");
#endif
        int padding_size = PACKET_LOWER_LIMIT_SIZE - sending_frame->payload_size;
        ptr += padding_size;
    }
    return size;
}
