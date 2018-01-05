#include <netinet/ip6.h>
#include <stdint.h>

uint32_t loop_checksum(uint32_t sum, uint16_t *ptr, uint32_t count);

uint16_t calc_checksum(uint8_t *header, int32_t length) {
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)header;

    for (int i = 0; i < length; i += 2) {
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

uint16_t calc_icmp6_checksum(struct ip6_hdr *ip6_header) {
    uint32_t sum = 0;

    uint16_t *ptr = (uint16_t *)&ip6_header->ip6_src;
    sum = loop_checksum(sum, ptr, 16);

    ptr = (uint16_t *)&ip6_header->ip6_dst;
    sum = loop_checksum(sum, ptr, 16);

    uint32_t payload_length = ip6_header->ip6_plen; /* 実ヘッダは16ビット、仮想ヘッダは32ビット */
    ptr = (uint16_t *)&payload_length;
    sum = loop_checksum(sum, ptr, 4);

    uint32_t next_header = htons(58);
    ptr = (uint16_t *)&next_header;
    sum = loop_checksum(sum, ptr, 4);

    ptr = (uint16_t *)(ip6_header + 1);
    sum = loop_checksum(sum, ptr, ntohs(ip6_header->ip6_plen));

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return ~sum;
}

uint32_t loop_checksum(uint32_t sum, uint16_t *ptr, uint32_t count) {
    for (int i = 0; i < count; i += 2) {
        sum += *ptr;
        if (sum & 0x80000000) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        ptr++;
    }
    return sum;
}
