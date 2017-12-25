#pragma once
#include <sys/epoll.h>

int init_event();
int add_event(int fd, int flag);
int modify_event(int fd, int flag);
struct epoll_event *wait_event();
