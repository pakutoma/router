#pragma once
#include <sys/epoll.h>

int init_event();
int add_event(int fd, int event);
int modify_event(int fd, int event);
struct epoll_event *wait_event();