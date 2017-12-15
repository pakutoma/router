#include "ether_frame.h"
void enqueue_send_queue(ether_frame_t *ether_frame);
ether_frame_t *dequeue_send_queue();
void print_send_queue();