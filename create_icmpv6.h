#pragma once
#include "ether_frame.h"

ether_frame_t *create_icmpv6_error(ether_frame_t *received_frame, uint8_t type, uint8_t code, uint32_t data);
void icmpv6_token_bucket_init();
void icmpv6_token_bucket_add_token();
