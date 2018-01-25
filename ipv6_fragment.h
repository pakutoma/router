#pragma once
#include "ether_frame.h"

void count_reassembly_time();
int init_reassembly_list();
bool exists_fragment_header(ether_frame_t *ether_frame);
ether_frame_t *reassemble_fragment_packet(ether_frame_t *ether_frame);
int send_large_frame(ether_frame_t *ether_frame, int mtu);
