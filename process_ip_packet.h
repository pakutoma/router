#pragma once
#include "settings.h"
#include "ether_frame.h"
#include <netinet/in.h>
int process_ip_packet(ether_frame_t *ether_frame);
