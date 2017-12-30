#include "ether_frame.h"
int init_send_queue();
void enqueue_send_queue(ether_frame_t *ether_frame);
int get_size_of_send_queue(int index);
struct mmsghdr *get_mmsghdrs(int index, int length);
