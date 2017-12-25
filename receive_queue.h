#pragma once
#define _GNU_SOURCE
#include <sys/socket.h>
#include "ether_frame.h"
ether_frame_t *dequeue_receive_queue();
int set_mmsghdrs(int index, struct mmsghdr *mmsg_hdrs, size_t length);
int enqueue_receive_queue(struct msghdr *msg_hdr);
int get_size_of_receive_queue(int index);
