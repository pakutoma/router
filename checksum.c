#include "crc32_table.h"
#include <stdint.h>
unsigned short calc_checksum(uint8_t *header, int32_t length) {
    uint32_t sum;
    uint16_t *ptr = (uint16_t *)header;
    int32_t i;

    sum = 0;
    ptr = (uint16_t *)header;

    for (i = 0; i < length; i += 2) {
        sum += (*ptr);
        if (sum & 0x80000000) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        ptr++;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}