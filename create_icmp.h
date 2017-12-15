#pragma once
#include "ether_frame.h"
#include <stdint.h>
ether_frame_t *create_time_exceeded_request(ether_frame_t *arrived_frame, uint32_t src_ipaddr, uint32_t dst_ipaddr);