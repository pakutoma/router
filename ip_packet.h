#pragma once
#include "device.h"
#include "ether_frame.h"
#include <netinet/in.h>
int process_ip_packet(ether_frame_t *ether_frame, device_t devices[NUMBER_OF_DEVICES], struct in_addr next_router);