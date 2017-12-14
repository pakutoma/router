#pragma once;
#include "ether_frame.h"
#include <stdint.h>
ether_frame_t *create_time_exceeded_request(uint8_t src_macaddr[8], uint8_t dst_macaddr[8], uint32_t src_ipaddr, uint32_t dst_ipaddr, uint8_t packet_data[64]) {