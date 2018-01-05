#pragma once
#include <stdint.h>
#include <netinet/ip6.h>
uint16_t calc_checksum(uint8_t *header, int32_t length);
uint16_t calc_icmp6_checksum(struct ip6_hdr *ip6_header);
